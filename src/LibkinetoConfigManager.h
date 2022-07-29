// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "LibkinetoTypes.h"

namespace dynolog {

class LibkinetoConfigManager {
 public:
  LibkinetoConfigManager();
  virtual ~LibkinetoConfigManager();

  int32_t registerLibkinetoContext(int64_t jobId, int32_t pid, int32_t gpu);
  static std::shared_ptr<LibkinetoConfigManager> getInstance();

  std::string getBaseConfig() {
    std::lock_guard<std::mutex> guard(mutex_);
    return baseConfig_;
  }

  std::string obtainOnDemandConfig(
      int64_t jobId,
      const std::vector<int32_t>& pids,
      int32_t configType);

  GpuProfilerResult setOnDemandConfig(
      int64_t jobId,
      const std::set<int32_t>& pids,
      const std::string& config,
      int32_t configType,
      int32_t limit);

  // Return the number of active libkineto processes
  // with the given Chronos / Tangram Job Id
  int processCount(int64_t jobId) const;

 protected:
  struct LibkinetoProcess {
    int32_t pid;
    std::chrono::system_clock::time_point lastRequestTime;
    std::string eventProfilerConfig;
    std::string activityProfilerConfig;
  };

  // A few callbacks for additional instrumentation.
  virtual void onRegisterProcess(const std::set<int32_t>& /*pids*/) {}

  virtual void preCheckOnDemandConfig(const LibkinetoProcess& /*process*/) {}

  virtual void onSetOnDemandConfig(const std::set<int32_t>& /*pids*/) {}

  virtual void onProcessCleanup(const std::set<int32_t>& /*pids*/) {}

  // Map of pid ancestry -> LibkinetoProcess
  using ProcessMap = std::map<std::set<int32_t>, LibkinetoProcess>;
  std::map<int64_t, ProcessMap> jobs_;

  // Map of gpu id -> pids
  using InstancesPerGpuMap = std::map<int32_t, std::set<int32_t>>;
  // Job id -> InstancesPerGpu
  std::map<int64_t, InstancesPerGpuMap> jobInstancesPerGpu_;
  mutable std::mutex mutex_;

  void setOnDemandConfigForProcess(
      GpuProfilerResult& res,
      LibkinetoProcess& process,
      const std::string& config,
      int32_t configType,
      int32_t limit);

 private:
  // Garbage collection and config refresh - periodically clean up
  // data from terminated processes.
  void start();
  void runGc();
  void refreshBaseConfig();

  std::string baseConfig_;
  std::thread* managerThread_{nullptr};
  std::atomic_bool stopFlag_{false};
  std::condition_variable managerCondVar_;
  // mutable std::mutex mutex_; TODO make private again
};

} // namespace dynolog
