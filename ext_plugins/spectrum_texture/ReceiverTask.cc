//
// Created by matthias on 8/19/24.
//

#include "include/spectrum_texture/ReceiverTask.h"

#include <csignal>
#include <utility>

#include "include/spectrum_texture/Definitions.h"

static thread_local volatile std::sig_atomic_t gIsRunning = true;

static void signalHandlerRecevierTask( int signum )
{
	std::cout << "ReceiverTask Spectrum: Interrupt signal (" << signum << ") received.\n";
	gIsRunning = false;
	std::cout << "ReceiverTask Spectrum: set running to false\n";
}

namespace spectrum {
  ReceiverTask::ReceiverTask(const std::string &fifoPath,
                             OnSpectrumReceived onSpectrumReceived)
      : fifoPath_(fifoPath), onSpectrumReceivedFn(std::move(onSpectrumReceived)), state(ReceiverTaskState::SyncHeader) {
    // FIFO is opened in start() on the receiver thread to avoid blocking the
    // main Flutter thread if no writer is present yet.
  }

  ReceiverTask::~ReceiverTask() {
    if (socket_fd >= 0) {
      close(socket_fd);
    }
  }

  size_t ReceiverTask::readSocket(size_t nBytes) {
    std::array<unsigned char, kDefaultBufferByteSize> r{};

    if (receiveBuffer.size() + nBytes >= kDefaultBufferByteSize) {
      receiveBuffer.erase(receiveBuffer.begin(), receiveBuffer.end());
    }

    while (gIsRunning) {
      ssize_t n = read(socket_fd, r.data(), nBytes);
      if (n < 0) {
        if (errno == EAGAIN) {
          usleep(1000);
          continue;
        } else if (errno == EINTR) {
          std::cout<<"ReceiverTask Spectrum: " << "receive from fifo interrupted!" << std::endl;
          gIsRunning = false;
        } else {
          std::cerr<<"ReceiverTask Spectrum: " << "receive from fifo failed! errno" << errno << " -> " << std::strerror(errno) << std::endl;
          gIsRunning = false;
          break;
        }
      } else if (n == 0) {
        // sleep and retry
        usleep(1000);
        continue;
      } else {
        receiveBuffer.insert(receiveBuffer.end(), r.begin(), r.begin() + n);
        return n;
      }
    }
    return -1;
  }

  void ReceiverTask::start() {
    struct sigaction act;
    bzero( &act, sizeof( act ) );
    act.sa_handler = signalHandlerRecevierTask;
    act.sa_flags   = 0;
    sigemptyset( &act.sa_mask );
    sigaction( SIGUSR1, &act, nullptr );

    // Open FIFO here in the receiver thread so the main Flutter thread is never
    // blocked waiting for a writer.  O_NONBLOCK lets us poll without hanging;
    // once the fd is obtained we switch back to blocking reads.
    while (gIsRunning) {
      socket_fd = open(fifoPath_.c_str(), O_RDONLY | O_NONBLOCK);
      if (socket_fd >= 0) {
        int flags = fcntl(socket_fd, F_GETFL, 0);
        fcntl(socket_fd, F_SETFL, flags & ~O_NONBLOCK);
        break;
      }
      if (errno == ENXIO) {
        // No writer yet — retry after a short sleep
        usleep(100000);
        continue;
      }
      std::cerr << "ReceiverTask Spectrum: Error opening fifo at " << fifoPath_ << ": " << std::strerror(errno) << std::endl;
      return;
    }
    if (!gIsRunning)
      return;

    state = ReceiverTaskState::SyncHeader;
    while (gIsRunning) {
      if (ReceiverTaskState::SyncHeader == state) {
        // sync on header ident
        if (readSocket(1) == -1) {
          break;
        };
        // check if we see the first chars of the header ident
        if (receiveBuffer.size() > sizeof(kSpectrumHeaderIdent)) {
          std::string needle(kSpectrumHeaderIdent);
          auto res = std::search(receiveBuffer.begin(), receiveBuffer.end(), needle.begin(), needle.end() + 1);
          if (res == receiveBuffer.end()) {
            // not found, just continue
          } else {
            // found, move to beginning of buffer
            receiveBuffer.erase(receiveBuffer.begin(), res);
            state = ReceiverTaskState::ReadHeader;
          }
        }
      } else if (ReceiverTaskState::ReadHeader == state) {
        if (readSocket(2) == -1) {
          break;
        };
        auto *header = reinterpret_cast<SpectrumDataHeader_t*>( receiveBuffer.data());
        expectedNbCoeff = header->numberCoefficients;

        if (expectedNbCoeff != presentNbCoeff ) {
          if (expectedNbCoeff * kBytesPerCoefficient > kDefaultBufferByteSize) {
            std::cerr<<"ReceiverTask Spectrum: " << "Maximum Message Size to big to store in buffer... exiting.." << std::endl;
            gIsRunning = false;
            break;
          }
          std::cerr<<"ReceiverTask Spectrum: " << "Number of expected Coefficients chanhed to " <<expectedNbCoeff<< std::endl;
          presentNbCoeff = expectedNbCoeff;
        }
        state = ReceiverTaskState::ReadData;
      } else { // ReceiverTaskState::ReadImage
        const auto bytesToRead = expectedNbCoeff*kBytesPerCoefficient;
        bool readFinish = false;
        size_t n = 0;
        size_t n_sum = 0;
        while (!readFinish) {
          n = readSocket(bytesToRead);
          n_sum += n;
          if (n == -1) {
            std::cerr<<"ReceiverTask Spectrum: " << "readSocket returned -1" << std::endl;
            break;
          };
          if (n_sum >= bytesToRead) {
            readFinish = true;
          }
        }
        // we have read all image bytesm, copy over
        onSpectrumReceivedFn(receiveBuffer.data() + sizeof(SpectrumDataHeader_t), bytesToRead);
        state = ReceiverTaskState::SyncHeader;
        // remove the part we wanted to read from socket buffer
        receiveBuffer.erase(receiveBuffer.begin(), receiveBuffer.begin() + bytesToRead);
      }
    }
    std::cout<<"ReceiverTask Spectrum: " << "ReceiverTask::start() ended" << std::endl;
  }
} // namespace acoustic_image
