//
// Created by matthias on 8/19/24.
//

#include "include/acoustic_image_texture/ReceiverTask.h"

#include <csignal>
#include <mutex>
#include <utility>

#include "include/acoustic_image_texture/Definitions.h"

static thread_local volatile std::sig_atomic_t gIsRunning = true;

static void signalHandlerRecevierTask( int signum )
{
	std::cout << "ReceiverTask: Interrupt signal (" << signum << ") received.\n";
	gIsRunning = false;
	std::cout << "ReceiverTask: set runing to false\n";
}

namespace acoustic_image
{
/**
 * Constructor for the ReceiverTask class.
 *
 * Initializes the ReceiverTask object with the given FIFO file path, image received callback,
 * and resolution changed callback. It sets up the task's internal state and opens the FIFO
 * file descriptor for reading.
 *
 * @param fifoPath The path to the FIFO file to be read.
 * @param onImageReceived The callback function to be invoked when an image is received.
 * @param onResolutionChangedCallback The callback function to be invoked when the resolution changes.
 *
 * @throws std::runtime_error If the FIFO file cannot be opened for reading.
 */
ReceiverTask::ReceiverTask( const std::string          &fifoPath,
                            OnImageReceived             onImageReceived,
                            OnResolutionChangedCallback onResolutionChangedCallback )
    : fifoPath_( fifoPath )
    , onResolutionChangedCallbackFn( std::move( onResolutionChangedCallback ) )
    , onImageReceivedFn( std::move( onImageReceived ) )
    , state( ReceiverTaskState::SyncHeader )
{
	// FIFO is opened in start() on the receiver thread to avoid blocking the
	// main Flutter thread if no writer is present yet.
}

ReceiverTask::~ReceiverTask()
{
	if ( socket_fd >= 0 )
	{
		close( socket_fd );
	}
}

size_t ReceiverTask::readSocket( size_t nBytes )
{

	{
		std::lock_guard<std::mutex> lock( bufMutex );
		if ( buf.size() + nBytes >= kDefaultBufferSize )
		{
			buf.erase( buf.begin(), buf.end() );
		}
	}

	while ( gIsRunning )
	{
		ssize_t n = read( socket_fd, receiveBuffer.data(), nBytes );
		if ( n < 0 )
		{
			if ( errno == EAGAIN )
			{
				usleep( 1000 );
				continue;
			}
			else if ( errno == EINTR )
			{
				std::cout<<"receive from fifo interrupted!"<<std::endl;
				gIsRunning = false;
			}
			else
			{
				std::cerr<<"receive from fifo failed! errno"<< errno<< " -> "<<std::strerror( errno )<<std::endl;
				gIsRunning = false;
				break;
			}
		}
		else if ( n == 0 )
		{
			// sleep and retry
			usleep( 1000 );
			continue;
		}
		else
		{
			{
				std::lock_guard<std::mutex> lock( bufMutex );
				buf.insert( buf.end(), receiveBuffer.begin(), receiveBuffer.begin() + n );
			}
			return n;
		}
	}
	return -1;
}

void ReceiverTask::start()
{
	struct sigaction act;
	bzero( &act, sizeof( act ) );
	act.sa_handler = signalHandlerRecevierTask;
	act.sa_flags   = 0;
	sigemptyset( &act.sa_mask );
	sigaction( SIGUSR1, &act, nullptr );

	// Open FIFO here in the receiver thread so the main Flutter thread is never
	// blocked waiting for a writer.  O_NONBLOCK lets us poll without hanging;
	// once the fd is obtained we switch back to blocking reads.
	while ( gIsRunning )
	{
		socket_fd = open( fifoPath_.c_str(), O_RDONLY | O_NONBLOCK );
		if ( socket_fd >= 0 )
		{
			int flags = fcntl( socket_fd, F_GETFL, 0 );
			fcntl( socket_fd, F_SETFL, flags & ~O_NONBLOCK );
			break;
		}
		if ( errno == ENXIO )
		{
			// No writer yet — retry after a short sleep
			usleep( 100000 );
			continue;
		}
		std::cerr << "Error opening fifo at " << fifoPath_ << ": " << std::strerror( errno ) << std::endl;
		return;
	}
	if ( !gIsRunning )
		return;

	state = ReceiverTaskState::SyncHeader;
	while ( gIsRunning )
	{
		if ( ReceiverTaskState::SyncHeader == state )
		{
			// sync on header ident
			if ( readSocket( 1 ) == -1 )
			{
				break;
			};
			// check if we see the first chars of the header ident
			if ( buf.size() > sizeof( kAcousticImageHeaderIdent ) )
			{
				std::string needle( kAcousticImageHeaderIdent );
				{
					std::lock_guard<std::mutex> lock( bufMutex );
					auto res = std::search( buf.begin(), buf.end(), needle.begin(), needle.end() + 1 );
					if ( res == buf.end() )
					{
						// not found, just continue
					}
					else
					{
						// found, move to beginning of buffer
						buf.erase( buf.begin(), res );
						state = ReceiverTaskState::ReadHeader;
					}
				}
			}
		}
		else if ( ReceiverTaskState::ReadHeader == state )
		{
			if ( readSocket( 4 ) == -1 )
			{
				break;
			};
			AcousticImageDataHeader_t *header;
			{
				std::lock_guard<std::mutex> lock( bufMutex );
				header = reinterpret_cast<AcousticImageDataHeader_t *>( buf.data() );
				expectedXRes = header->xResolution;
				expectedYRes = header->yResolution;
			}
			if ( expectedXRes != presentXRes || expectedYRes != presentYRes )
			{
				if (expectedXRes * expectedYRes *kBytesPerPixel > kDefaultBufferSize){
					std::cerr<<"Maximum Message Size to big to store in buffer... exiting.."<<std::endl;
					gIsRunning = false;
					break;
				}
				onResolutionChangedCallbackFn( expectedXRes, expectedYRes );
				presentXRes = expectedXRes;
				presentYRes = expectedYRes;
			}
			state = ReceiverTaskState::ReadImage;
		}
		else if ( ReceiverTaskState::ReadImage == state )
		{ // ReceiverTaskState::ReadImage
			const auto bytesToRead = expectedXRes * expectedYRes * kBytesPerPixel;
			bool       readFinish  = false;
			size_t     n           = 0;
			size_t     n_sum       = 0;
			while ( !readFinish )
			{
				n = readSocket( bytesToRead );
				n_sum += n;
				if ( n == -1 )
				{
					std::cerr<< "readSocket returned -1"<<std::endl;
					break;
				};
				if ( n_sum >= bytesToRead )
				{
					readFinish = true;
				}
			}
			// we have read all image bytesm, copy over
			{
				{
					std::lock_guard<std::mutex> lock( bufMutex );
					onImageReceivedFn( buf.data() + sizeof( AcousticImageDataHeader_t ), bytesToRead );
				}

				state = ReceiverTaskState::SyncHeader;
					// Remove the header AND image bytes we consumed so the buffer
					// stays clean for the next SyncHeader iteration.
					{
						std::lock_guard<std::mutex> lock( bufMutex );
						const size_t toErase = sizeof( AcousticImageDataHeader_t ) + bytesToRead;
						if ( toErase <= buf.size() )
							buf.erase( buf.begin(), buf.begin() + toErase );
						else
							buf.clear();
					}
			}
		}
	}
	std::cout<< "ReceiverTask::start() ended" <<std::endl;
}
} // namespace acoustic_image
