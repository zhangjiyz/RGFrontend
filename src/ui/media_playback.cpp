#include "ui/media_playback.h"

#include <SDL.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <limits>
#include <string>
#include <thread>
#include <vector>

#ifndef _WIN32
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
#endif

namespace mpl {

namespace {

namespace fs = std::filesystem;

struct PipeProcess {
  int pid = -1;
  int output_fd = -1;
};

constexpr const char *kFfmpegLibraryPath =
    "/usr/lib/aarch64-linux-gnu:/lib/aarch64-linux-gnu:/usr/lib:/lib";
constexpr const char *kH700FontconfigPreload =
    "/usr/lib/aarch64-linux-gnu/libfontconfig.so.1.12.0";
constexpr const char *kVideoFfmpegLogPath = "/tmp/mpl-video-ffmpeg.log";
constexpr const char *kAudioFfmpegLogPath = "/tmp/mpl-audio-ffmpeg.log";

bool FileExists(const std::string &path) {
  std::error_code error;
  return !path.empty() && fs::is_regular_file(fs::u8path(path), error);
}

#ifndef _WIN32
void MarkOpenDescriptorsCloseOnExec() {
  fs::path fd_dir{"/proc/self/fd"};
  std::error_code error;
  if (!fs::is_directory(fd_dir, error)) return;
  for (const fs::directory_entry &entry : fs::directory_iterator(fd_dir, error)) {
    if (error) break;
    const std::string name = entry.path().filename().string();
    char *end = nullptr;
    const long fd = std::strtol(name.c_str(), &end, 10);
    if (end == name.c_str() || *end != '\0' || fd < 3) continue;
    const int flags = fcntl(static_cast<int>(fd), F_GETFD);
    if (flags >= 0) fcntl(static_cast<int>(fd), F_SETFD, flags | FD_CLOEXEC);
  }
}

void TerminateProcessGroup(int pid) {
  if (pid <= 0) return;
  kill(-pid, SIGTERM);
  for (int attempt = 0; attempt < 30; ++attempt) {
    const pid_t result = waitpid(static_cast<pid_t>(pid), nullptr, WNOHANG);
    if (result == pid || (result < 0 && errno == ECHILD)) return;
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  kill(-pid, SIGKILL);
  while (waitpid(static_cast<pid_t>(pid), nullptr, 0) < 0 && errno == EINTR) {}
}

std::vector<std::string> FfmpegEnvironment() {
  std::vector<std::string> env;
  for (char **cursor = environ; cursor && *cursor; ++cursor) {
    const std::string entry(*cursor);
    if (entry.rfind("LD_LIBRARY_PATH=", 0) == 0 ||
        entry.rfind("LD_PRELOAD=", 0) == 0) {
      continue;
    }
    env.push_back(entry);
  }
  env.push_back(std::string("LD_LIBRARY_PATH=") + kFfmpegLibraryPath);
  if (FileExists(kH700FontconfigPreload)) {
    env.push_back(std::string("LD_PRELOAD=") + kH700FontconfigPreload);
  }
  return env;
}

bool SpawnPipeProcess(const std::vector<std::string> &arguments,
                      const char *stderr_path, PipeProcess *process) {
  if (!process || arguments.empty()) return false;
  MarkOpenDescriptorsCloseOnExec();
  int pipe_fds[2] = {-1, -1};
  if (pipe(pipe_fds) != 0) return false;
  for (int fd : pipe_fds) {
    const int flags = fcntl(fd, F_GETFD);
    if (flags >= 0) fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
  }

  posix_spawn_file_actions_t actions;
  if (posix_spawn_file_actions_init(&actions) != 0) {
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    return false;
  }
  posix_spawn_file_actions_adddup2(&actions, pipe_fds[1], STDOUT_FILENO);
  posix_spawn_file_actions_addclose(&actions, pipe_fds[0]);
  posix_spawn_file_actions_addclose(&actions, pipe_fds[1]);
  posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
  posix_spawn_file_actions_addopen(&actions, STDERR_FILENO,
                                   stderr_path ? stderr_path : "/dev/null",
                                   O_WRONLY | O_CREAT | O_TRUNC, 0644);

  posix_spawnattr_t attributes;
  if (posix_spawnattr_init(&attributes) != 0) {
    posix_spawn_file_actions_destroy(&actions);
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    return false;
  }
  posix_spawnattr_setflags(&attributes, POSIX_SPAWN_SETPGROUP);
  posix_spawnattr_setpgroup(&attributes, 0);

  std::vector<char *> argv;
  argv.reserve(arguments.size() + 1);
  for (const std::string &argument : arguments) {
    argv.push_back(const_cast<char *>(argument.c_str()));
  }
  argv.push_back(nullptr);

  std::vector<std::string> env_strings = FfmpegEnvironment();
  std::vector<char *> envp;
  envp.reserve(env_strings.size() + 1);
  for (std::string &entry : env_strings) {
    envp.push_back(const_cast<char *>(entry.c_str()));
  }
  envp.push_back(nullptr);

  pid_t child = -1;
  const int result = posix_spawnp(&child, argv[0], &actions, &attributes,
                                  argv.data(), envp.data());
  posix_spawnattr_destroy(&attributes);
  posix_spawn_file_actions_destroy(&actions);
  close(pipe_fds[1]);
  if (result != 0) {
    close(pipe_fds[0]);
    return false;
  }

  process->pid = static_cast<int>(child);
  process->output_fd = pipe_fds[0];
  return true;
}

void ClosePipeProcess(PipeProcess *process) {
  if (!process) return;
  if (process->output_fd >= 0) {
    close(process->output_fd);
    process->output_fd = -1;
  }
  TerminateProcessGroup(process->pid);
  process->pid = -1;
}

ssize_t ReadNoEintr(int fd, void *buffer, std::size_t size) {
  for (;;) {
    const ssize_t bytes = read(fd, buffer, size);
    if (bytes < 0 && errno == EINTR) continue;
    return bytes;
  }
}

void SetNonBlocking(int fd) {
  if (fd < 0) return;
  const int flags = fcntl(fd, F_GETFL);
  if (flags >= 0) fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
#endif

std::vector<std::string> VideoArguments(const std::string &path, int width, int height,
                                        bool loop) {
  std::vector<std::string> args = {"ffmpeg", "-nostdin", "-loglevel", "error"};
  if (loop) {
    args.push_back("-stream_loop");
    args.push_back("-1");
  }
  args.push_back("-i");
  args.push_back(path);
  args.push_back("-an");
  args.push_back("-vf");
  args.push_back("scale=" + std::to_string(width) + ":" + std::to_string(height) +
                 ":force_original_aspect_ratio=decrease,"
                 "pad=" + std::to_string(width) + ":" + std::to_string(height) +
                 ":(ow-iw)/2:(oh-ih)/2,format=rgba");
  args.push_back("-r");
  args.push_back("12");
  args.push_back("-f");
  args.push_back("rawvideo");
  args.push_back("pipe:1");
  return args;
}

std::vector<std::string> AudioArguments(const std::string &path, bool loop) {
  std::vector<std::string> args = {"ffmpeg", "-nostdin", "-loglevel", "error"};
  if (loop) {
    args.push_back("-stream_loop");
    args.push_back("-1");
  }
  args.push_back("-i");
  args.push_back(path);
  args.push_back("-map");
  args.push_back("0:a:0?");
  args.push_back("-vn");
  args.push_back("-acodec");
  args.push_back("pcm_s16le");
  args.push_back("-f");
  args.push_back("s16le");
  args.push_back("-ac");
  args.push_back("2");
  args.push_back("-ar");
  args.push_back("44100");
  args.push_back("pipe:1");
  return args;
}

void ApplyFadeIn(std::vector<std::uint8_t> *buffer, std::uint64_t *fade_frame,
                 std::int16_t *last_left, std::int16_t *last_right) {
  if (!buffer || !fade_frame || !last_left || !last_right || buffer->empty()) return;
  constexpr std::uint64_t kFadeFrames = 882;
  constexpr std::size_t kChannels = 2;
  const std::size_t sample_count = buffer->size() / sizeof(std::int16_t);
  std::int16_t *samples = reinterpret_cast<std::int16_t *>(buffer->data());
  const std::size_t frames = sample_count / kChannels;
  for (std::size_t frame = 0; frame < frames; ++frame) {
    if (*fade_frame < kFadeFrames) {
      const std::int32_t gain = static_cast<std::int32_t>(*fade_frame);
      for (std::size_t channel = 0; channel < kChannels; ++channel) {
        const std::size_t index = frame * kChannels + channel;
        samples[index] = static_cast<std::int16_t>(
            static_cast<std::int32_t>(samples[index]) * gain /
            static_cast<std::int32_t>(kFadeFrames));
      }
      ++(*fade_frame);
    }
    *last_left = samples[frame * kChannels];
    *last_right = samples[frame * kChannels + 1];
  }
}

void QueueFadeToSilence(SDL_AudioDeviceID device, std::int16_t left, std::int16_t right) {
  if (device == 0 && left == 0 && right == 0) return;
  constexpr std::size_t kChannels = 2;
  constexpr std::size_t kFadeFrames = 882;
  std::vector<std::int16_t> fade(kFadeFrames * kChannels);
  for (std::size_t frame = 0; frame < kFadeFrames; ++frame) {
    const std::int32_t remaining = static_cast<std::int32_t>(kFadeFrames - frame);
    fade[frame * kChannels] = static_cast<std::int16_t>(
        static_cast<std::int32_t>(left) * remaining /
        static_cast<std::int32_t>(kFadeFrames));
    fade[frame * kChannels + 1] = static_cast<std::int16_t>(
        static_cast<std::int32_t>(right) * remaining /
        static_cast<std::int32_t>(kFadeFrames));
  }
  SDL_QueueAudio(device, fade.data(),
                 static_cast<Uint32>(fade.size() * sizeof(std::int16_t)));
  const Uint32 started = SDL_GetTicks();
  while (SDL_GetQueuedAudioSize(device) > 0 &&
         !SDL_TICKS_PASSED(SDL_GetTicks(), started + 60)) {
    SDL_Delay(4);
  }
  SDL_ClearQueuedAudio(device);
}

}  // namespace

VideoPreviewDecoder::~VideoPreviewDecoder() {
  Stop();
}

void VideoPreviewDecoder::Start(std::string path, int width, int height, bool loop) {
  if (path == path_ && loop == loop_) return;
  Stop();
  if (!FileExists(path) || width <= 0 || height <= 0) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      latest_frame_.clear();
      ++version_;
    }
    path_.clear();
    loop_ = false;
    return;
  }
  path_ = path;
  loop_ = loop;
  stop_.store(false);
  thread_ = std::thread(&VideoPreviewDecoder::Worker, this, std::move(path), width, height, loop);
}

void VideoPreviewDecoder::Stop() {
  stop_.store(true);
  const int pid = child_pid_.load();
#ifndef _WIN32
  TerminateProcessGroup(pid);
#else
  (void)pid;
#endif
  if (thread_.joinable()) thread_.join();
  child_pid_.store(-1);
  {
    std::lock_guard<std::mutex> lock(mutex_);
    latest_frame_.clear();
    ++version_;
  }
  path_.clear();
  loop_ = false;
}

bool VideoPreviewDecoder::CopyLatestFrame(std::vector<std::uint8_t> *pixels,
                                          std::uint64_t *version) {
  if (!pixels || !version) return false;
  std::lock_guard<std::mutex> lock(mutex_);
  if (latest_frame_.empty() || *version == version_) return false;
  *pixels = latest_frame_;
  *version = version_;
  return true;
}

void VideoPreviewDecoder::Worker(std::string path, int width, int height, bool loop) {
#ifdef _WIN32
  (void)path;
  (void)width;
  (void)height;
  (void)loop;
#else
  PipeProcess process;
  if (!SpawnPipeProcess(VideoArguments(path, width, height, loop),
                        kVideoFfmpegLogPath, &process)) {
    return;
  }
  child_pid_.store(process.pid);
  const std::size_t frame_size = static_cast<std::size_t>(width) *
                                 static_cast<std::size_t>(height) * 4;
  std::vector<std::uint8_t> frame(frame_size);
  std::size_t offset = 0;
  constexpr auto kFramePeriod = std::chrono::microseconds(83333);
  auto next_frame_at = std::chrono::steady_clock::now();
  while (!stop_.load()) {
    const ssize_t bytes = ReadNoEintr(process.output_fd, frame.data() + offset,
                                      frame.size() - offset);
    if (bytes <= 0) break;
    offset += static_cast<std::size_t>(bytes);
    if (offset != frame.size()) continue;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      latest_frame_ = frame;
      ++version_;
    }
    offset = 0;
    next_frame_at += kFramePeriod;
    while (!stop_.load()) {
      const auto now = std::chrono::steady_clock::now();
      if (now >= next_frame_at) break;
      const auto remaining = next_frame_at - now;
      std::this_thread::sleep_for(std::min(
          remaining, std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                         std::chrono::milliseconds(5))));
    }
    const auto now = std::chrono::steady_clock::now();
    if (now > next_frame_at + kFramePeriod * 2) next_frame_at = now;
  }
  ClosePipeProcess(&process);
  child_pid_.store(-1);
#endif
}

AudioPreviewPlayer::~AudioPreviewPlayer() {
  Shutdown();
}

void AudioPreviewPlayer::SetDeviceOpenedCallback(std::function<void()> callback) {
  std::lock_guard<std::mutex> lock(mutex_);
  device_opened_callback_ = std::move(callback);
}

void AudioPreviewPlayer::Start(std::string path, bool loop) {
  if (!FileExists(path)) {
    Stop();
    return;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (!thread_.joinable()) {
    shutdown_ = false;
    thread_ = std::thread(&AudioPreviewPlayer::Worker, this);
  }
  if (path == requested_path_ && loop == requested_loop_) return;
  requested_path_ = path;
  requested_loop_ = loop;
  path_ = path;
  loop_ = loop;
  ++request_version_;
  request_cv_.notify_one();
}

void AudioPreviewPlayer::Stop() {
  std::unique_lock<std::mutex> lock(mutex_);
  path_.clear();
  loop_ = false;
  if (!thread_.joinable()) return;
  if (requested_path_.empty() && ack_version_ == request_version_) return;
  requested_path_.clear();
  requested_loop_ = false;
  const std::uint64_t request = ++request_version_;
  request_cv_.notify_one();
  ack_cv_.wait_for(lock, std::chrono::milliseconds(500), [this, request] {
    return ack_version_ >= request || shutdown_;
  });
}

void AudioPreviewPlayer::Shutdown() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    path_.clear();
    loop_ = false;
    requested_path_.clear();
    requested_loop_ = false;
    shutdown_ = true;
    ++request_version_;
    request_cv_.notify_one();
  }
  const int pid = child_pid_.load();
#ifndef _WIN32
  TerminateProcessGroup(pid);
#else
  (void)pid;
#endif
  if (thread_.joinable()) thread_.join();
  child_pid_.store(-1);
}

void AudioPreviewPlayer::Worker() {
#ifdef _WIN32
#else
  SDL_AudioSpec desired{};
  desired.freq = 44100;
  desired.format = AUDIO_S16SYS;
  desired.channels = 2;
  desired.samples = 2048;
  SDL_AudioSpec obtained{};
  const SDL_AudioDeviceID device = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
  if (device == 0) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!shutdown_) {
      ack_version_ = request_version_;
      ack_cv_.notify_all();
      const std::uint64_t version = request_version_;
      request_cv_.wait(lock, [this, version] {
        return shutdown_ || request_version_ != version;
      });
    }
    return;
  }
  std::function<void()> device_opened_callback;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    device_opened_callback = device_opened_callback_;
  }
  if (device_opened_callback) device_opened_callback();
  SDL_PauseAudioDevice(device, 0);
  PipeProcess process;
  std::vector<std::uint8_t> buffer(8192);
  std::uint64_t fade_in_frame = 0;
  std::int16_t last_left = 0;
  std::int16_t last_right = 0;
  std::uint64_t handled_version = std::numeric_limits<std::uint64_t>::max();
  const Uint32 max_queued = static_cast<Uint32>(
      std::max(8192, obtained.freq * static_cast<int>(obtained.channels) * 2 / 5));

  for (;;) {
    std::string requested_path;
    bool requested_loop = false;
    std::uint64_t requested_version = 0;
    bool shutdown = false;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      requested_path = requested_path_;
      requested_loop = requested_loop_;
      requested_version = request_version_;
      shutdown = shutdown_;
    }

    if (requested_version != handled_version) {
      if (process.pid > 0 || last_left != 0 || last_right != 0) {
        QueueFadeToSilence(device, last_left, last_right);
      } else {
        SDL_ClearQueuedAudio(device);
      }
      last_left = 0;
      last_right = 0;
      fade_in_frame = 0;
      ClosePipeProcess(&process);
      child_pid_.store(-1);
      if (!requested_path.empty() && !shutdown &&
          SpawnPipeProcess(AudioArguments(requested_path, requested_loop),
                           kAudioFfmpegLogPath, &process)) {
        SetNonBlocking(process.output_fd);
        child_pid_.store(process.pid);
      }
      handled_version = requested_version;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        ack_version_ = handled_version;
        ack_cv_.notify_all();
      }
    }
    if (shutdown) break;

    if (process.output_fd < 0) {
      std::unique_lock<std::mutex> lock(mutex_);
      request_cv_.wait_for(lock, std::chrono::milliseconds(20), [this, handled_version] {
        return shutdown_ || request_version_ != handled_version;
      });
      continue;
    }

    while (SDL_GetQueuedAudioSize(device) > max_queued) {
      bool changed = false;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        changed = shutdown_ || request_version_ != handled_version;
      }
      if (changed) break;
      SDL_Delay(8);
    }
    const ssize_t bytes = ReadNoEintr(process.output_fd, buffer.data(), buffer.size());
    if (bytes == 0) {
      QueueFadeToSilence(device, last_left, last_right);
      last_left = 0;
      last_right = 0;
      fade_in_frame = 0;
      ClosePipeProcess(&process);
      child_pid_.store(-1);
      continue;
    }
    if (bytes < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        SDL_Delay(4);
        continue;
      }
      ClosePipeProcess(&process);
      child_pid_.store(-1);
      continue;
    }
    const std::size_t usable_bytes = static_cast<std::size_t>(bytes) -
                                     static_cast<std::size_t>(bytes) % 4;
    if (usable_bytes == 0) {
      SDL_Delay(1);
      continue;
    }
    buffer.resize(usable_bytes);
    ApplyFadeIn(&buffer, &fade_in_frame, &last_left, &last_right);
    SDL_QueueAudio(device, buffer.data(), static_cast<Uint32>(usable_bytes));
    buffer.resize(8192);
  }

  QueueFadeToSilence(device, last_left, last_right);
  ClosePipeProcess(&process);
  child_pid_.store(-1);
  SDL_ClearQueuedAudio(device);
  SDL_CloseAudioDevice(device);
#endif
}

}  // namespace mpl
