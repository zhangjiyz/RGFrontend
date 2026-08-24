#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace mpl {

class VideoPreviewDecoder {
 public:
  VideoPreviewDecoder() = default;
  ~VideoPreviewDecoder();

  VideoPreviewDecoder(const VideoPreviewDecoder &) = delete;
  VideoPreviewDecoder &operator=(const VideoPreviewDecoder &) = delete;

  void Start(std::string path, int width, int height, bool loop);
  void Stop();
  bool CopyLatestFrame(std::vector<std::uint8_t> *pixels, std::uint64_t *version);
  const std::string &Path() const { return path_; }
  bool Loop() const { return loop_; }

 private:
  void Worker(std::string path, int width, int height, bool loop);

  std::thread thread_;
  std::atomic<bool> stop_{false};
  std::atomic<int> child_pid_{-1};
  std::mutex mutex_;
  std::vector<std::uint8_t> latest_frame_;
  std::uint64_t version_ = 0;
  std::string path_;
  bool loop_ = false;
};

class AudioPreviewPlayer {
 public:
  AudioPreviewPlayer() = default;
  ~AudioPreviewPlayer();

  AudioPreviewPlayer(const AudioPreviewPlayer &) = delete;
  AudioPreviewPlayer &operator=(const AudioPreviewPlayer &) = delete;

  void Start(std::string path, bool loop);
  void Stop();
  void Shutdown();
  void SetDeviceOpenedCallback(std::function<void()> callback);
  const std::string &Path() const { return path_; }
  bool Loop() const { return loop_; }

 private:
  void Worker();

  std::thread thread_;
  std::atomic<int> child_pid_{-1};
  std::mutex mutex_;
  std::condition_variable request_cv_;
  std::condition_variable ack_cv_;
  std::string requested_path_;
  bool requested_loop_ = false;
  std::uint64_t request_version_ = 0;
  std::uint64_t ack_version_ = 0;
  bool shutdown_ = false;
  std::function<void()> device_opened_callback_;
  std::string path_;
  bool loop_ = false;
};

}  // namespace mpl
