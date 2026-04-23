#ifndef FLUTTER_ELINUX_SPECTRUM_RECEIVERTASK_H
#define FLUTTER_ELINUX_SPECTRUM_RECEIVERTASK_H

#include <array>
#include <cmath>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <thread>

#include <fcntl.h>
#include <unistd.h>
#include "Definitions.h"

namespace spectrum {

  enum class ReceiverTaskState {
    SyncHeader, ReadHeader, ReadData
  };

  class ReceiverTask {
  public:
    ReceiverTask(const std::string &fifoPath,
                 OnSpectrumReceived onSpectrumReceived);

    virtual ~ReceiverTask();

    void start();

  private:
    std::string fifoPath_;
    uint presentNbCoeff = kMaxCoefficient;
    uint expectedNbCoeff = 512;
    int socket_fd = -1;
    std::thread t;
    OnSpectrumReceived onSpectrumReceivedFn;
    ReceiverTaskState state;
    std::vector<unsigned char> receiveBuffer{};

    // tries to read nBytes from socket, returns actually read byte count
    size_t readSocket(size_t nBytes);
  };

} // namespace

#endif // FLUTTER_ELINUX_SPECTRUM_RECEIVERTASK_H
