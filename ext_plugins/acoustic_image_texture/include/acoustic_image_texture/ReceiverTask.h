//
// Created by matthias on 8/19/24.
//

#ifndef FLUTTER_ELINUX_RECEIVERTASK_H
#define FLUTTER_ELINUX_RECEIVERTASK_H

#include "Definitions.h"

#include <array>
#include <cmath>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <numeric>
#include <thread>

#include <fcntl.h>
#include <unistd.h>

namespace acoustic_image {

    enum class ReceiverTaskState { SyncHeader, ReadHeader, ReadImage};

    class ReceiverTask {
    public:
        ReceiverTask(const std::string &fifoPath,
                     OnImageReceived onImageReceived,
                     OnResolutionChangedCallback onResolutionChangedCallback);

        virtual ~ReceiverTask();

        void start();

    private:
		std::string fifoPath_;
		uint presentXRes = 100;
		uint presentYRes = 75;
        uint expectedXRes = 0;
        uint expectedYRes = 0;
        int socket_fd = -1;
        std::thread t;
        OnResolutionChangedCallback onResolutionChangedCallbackFn;
        OnImageReceived onImageReceivedFn;
        ReceiverTaskState state;
        std::vector<unsigned char> buf;
        // Heap-allocated receive buffer (avoids 3.5 MB stack frame in readSocket)
        std::array<unsigned char, kDefaultBufferSize> receiveBuffer{};

        // tries to read nBytes from socket, returns actually read byte count
        size_t readSocket(size_t nBytes);
	    std::mutex bufMutex{} ;
    };

} // namespace

#endif // FLUTTER_ELINUX_RECEIVERTASK_H
