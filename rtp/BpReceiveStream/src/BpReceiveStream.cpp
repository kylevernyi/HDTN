#include "BpReceiveStream.h"
#include "Logger.h"
#include <boost/process.hpp>

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

#define FFMPEG_SDP_HEADER "data:application/sdp;charset=UTF-8,"

BpReceiveStream::BpReceiveStream(size_t numCircularBufferVectors, const std::string& remoteHostname, const uint16_t remotePort, uint16_t maxOutgoingRtpPacketSizeBytes, std::string ffmpegCommand) : BpSinkPattern(), 
        m_numCircularBufferVectors(numCircularBufferVectors),
        m_outgoingRtpPort(remotePort),
        m_maxOutgoingRtpPacketSizeBytes(maxOutgoingRtpPacketSizeBytes),
        socket(io_service),
        m_ffmpegCommand(ffmpegCommand)
{
    m_maxOutgoingRtpPayloadSizeBytes = m_maxOutgoingRtpPacketSizeBytes - sizeof(rtp_header);
    m_outgoingDtnRtpPtr = std::make_shared<DtnRtp>(UINT64_MAX);
    m_processingThread = boost::make_unique<boost::thread>(boost::bind(&BpReceiveStream::ProcessIncomingBundlesThread, this)); 
    
    m_incomingCircularPacketQueue.set_capacity(m_numCircularBufferVectors);

    m_udpBatchSenderPtr = std::make_shared<UdpBatchSender>();
    m_udpBatchSenderPtr->SetOnSentPacketsCallback(boost::bind(&BpReceiveStream::OnSentRtpPacketCallback, this, 
            boost::placeholders::_1, boost::placeholders::_2, boost::placeholders::_3));
    m_udpBatchSenderPtr->Init(remoteHostname, remotePort);
    m_udpEndpoint = m_udpBatchSenderPtr->GetCurrentUdpEndpoint();
   
    socket.open(boost::asio::ip::udp::v4());

}

BpReceiveStream::~BpReceiveStream()
{
    m_running = false;

    m_udpBatchSenderPtr->Stop();
    Stop();

    LOG_INFO(subprocess) << "m_totalRtpPacketsReceived: " << m_totalRtpPacketsReceived;
    LOG_INFO(subprocess) << "m_totalRtpPacketsSent: " << m_totalRtpPacketsSent;
    LOG_INFO(subprocess) << "m_totalRtpPacketsQueued: " << m_totalRtpPacketsQueued;
    LOG_INFO(subprocess) << "m_totalRtpPacketFailedToSend: " << m_totalRtpPacketsFailedToSend;
    LOG_INFO(subprocess) << "m_incomingCircularPacketQueue.size(): " << m_incomingCircularPacketQueue.size();
}

void BpReceiveStream::ProcessIncomingBundlesThread()
{
    static const boost::posix_time::time_duration timeout(boost::posix_time::milliseconds(250));
    static bool firstPacket = true;

    std::vector<uint8_t> rtpFrame;
    rtpFrame.reserve(m_maxOutgoingRtpPacketSizeBytes);

    while (m_running) 
    {
        bool notInWaitForNewPacketState = TryWaitForIncomingDataAvailable(timeout);
        if (notInWaitForNewPacketState) {

            boost::mutex::scoped_lock lock(m_incomingQueueMutex);
            
            padded_vector_uint8_t &incomingPacket = m_incomingCircularPacketQueue.front(); // process front of queue

            // split front of incoming queue into the minimum number of RTP packets possible
            size_t totalBytesPayload = incomingPacket.size() - sizeof(rtp_header) ; // get size of the incoming rtp frame payload
            size_t bytesPayloadRemaining = totalBytesPayload; 
            double packetsToTransfer = ((double) totalBytesPayload) / ((double) m_maxOutgoingRtpPacketSizeBytes);
            size_t numPacketsToTransferPayload = ceil(packetsToTransfer); 


            // use these to export to UdpBatchSend
            // std::shared_ptr<std::vector<UdpSendPacketInfo> > udpSendPacketInfoVecPtr = 
                    // std::make_shared<std::vector<UdpSendPacketInfo> >(numPacketsToTransferPayload);  
            // std::vector<UdpSendPacketInfo>& udpSendPacketInfoVec = *udpSendPacketInfoVecPtr;

            // this is where we copy incoming frames into, reserve max space right now for efficiency
            // std::vector<std::vector<uint8_t>> rtpFrameVec(numPacketsToTransferPayload);
            // for (auto vec=rtpFrameVec.begin(); vec!=rtpFrameVec.end(); vec++)  {
                // vec->reserve(m_maxOutgoingRtpPacketSizeBytes);
            // }
            
            // LOG_DEBUG(subprocess) << "Sending " << numPacketsToTransferPayload  << " packets";

            if (firstPacket) {
                firstPacket = false;
                m_outgoingDtnRtpPtr->UpdateHeader((rtp_header *) m_incomingCircularPacketQueue.front().data(), USE_INCOMING_SEQ); // starts us at same sequence number
            }

            size_t offset = sizeof(rtp_header); // start first byte after header 

            size_t nextPacketSize;

            for (size_t i = 0; i < numPacketsToTransferPayload; i++)
            {
                if ( (bytesPayloadRemaining + sizeof(rtp_header)) >= m_maxOutgoingRtpPacketSizeBytes) {
                    nextPacketSize = m_maxOutgoingRtpPacketSizeBytes;
                } else {
                    nextPacketSize = bytesPayloadRemaining + sizeof(rtp_header);
                }
                size_t bytesPayloadToCopy = nextPacketSize - sizeof(rtp_header);
                
                rtpFrame.resize(nextPacketSize);

                // update header (everything execpt sequence number & marker bit from the incoming packet)
                m_outgoingDtnRtpPtr->UpdateHeader((rtp_header *) incomingPacket.data(), USE_INCOMING_SEQ);
                // copy outgoing RtpDtn session header into every frame
                memcpy(rtpFrame.data(), m_outgoingDtnRtpPtr->GetHeader(), sizeof(rtp_header));
                m_outgoingDtnRtpPtr->IncSequence(); // for next frame


                // copy incrementing payload into frame
                memcpy(rtpFrame.data()+sizeof(rtp_header), incomingPacket.data()+offset, bytesPayloadToCopy);
                
                SendUdpPacket(rtpFrame);
                offset += bytesPayloadToCopy;

                // std::cout << "frame" << std::endl;
                // rtp_frame * frame = (rtp_frame * ) rtpFrame.data();
                // frame->print_header();
                
                // std::cout << rtpFrameVec.size() << " packets" << std::endl;
                bytesPayloadRemaining -= bytesPayloadToCopy;  // only take off the payload bytes

                // append to outgoing data (cheap "copy")
                // udpSendPacketInfoVec[i].constBufferVec.resize(1);
                // udpSendPacketInfoVec[i].constBufferVec[0] = boost::asio::buffer(rtpFrameVec[i]);

            }
    // std::cout << rtpFrameVec.size() << " packets" << std::endl;
            // we need to ensure that the batch sender actually sends our data before exiting so it is not lost
            // {
            //     m_sentPacketsSuccess = false;
            //     boost::mutex::scoped_lock lock(m_sentPacketsMutex); //must lock before checking the flag

            //     m_udpBatchSenderPtr->QueueSendPacketsOperation_ThreadSafe(std::move(udpSendPacketInfoVecPtr), numPacketsToTransferPayload); // data is stolen from rtpFrameVec

            //     while (m_sentPacketsSuccess == false) {
            //         LOG_DEBUG(subprocess) << "Waiting for sucessful send";
            //         m_cvSentPacket.timed_wait(lock, timeout);
            //         LOG_DEBUG(subprocess) << "Got sucessful send";
            //     }
            // }
            // SendUdpPacket(rtpFrameVec);
            
            // LOG_DEBUG(subprocess) << "Popping";
            m_incomingCircularPacketQueue.pop_front();

        }
    }
}

// Data from BpSourcePattern comes in through here
bool BpReceiveStream::ProcessPayload(const uint8_t *data, const uint64_t size)
{
    padded_vector_uint8_t vec(size);
    memcpy(vec.data(), data, size);
    if (vec.at(0) == SDP_FILE_STR_HEADER) 
    {
        LOG_INFO(subprocess) << "Got SDP File information";
        std::string sdpFile((char * )&vec[1], size - sizeof(uint8_t));
        LOG_INFO(subprocess) << "Sdp File: \n" << sdpFile;
        
        if (ReadSdpFileFromString(sdpFile) == 0) 
        {
            ExecuteFFmpegInstance();
        }

    }

    {
        boost::mutex::scoped_lock lock(m_incomingQueueMutex); // lock mutex 
        m_incomingCircularPacketQueue.push_back(std::move(vec));
        m_totalRtpPacketsQueued++;
    }
    m_incomingQueueCv.notify_one();
    m_totalRtpPacketsReceived++;

    // rtp_frame * frame= (rtp_frame *) data;
    // LOG_DEBUG(subprocess) << "Got bundle of size " << size;
    // frame->print_header();
    return true;
}

// we need to change the port number to the outgoing rtp port
int BpReceiveStream::ReadSdpFileFromString(std::string sdpFileString)
{
    size_t mediaDescriptorLocation;
    size_t rtpDescriptorLocation = sdpFileString.find("RTP"); // sdp protocol
    
    // check for video stream first
    mediaDescriptorLocation = sdpFileString.find("m=video"); // this is sdp protocol
    if (mediaDescriptorLocation == std::string::npos) // if no video stream found, check for audio stream
    {
        mediaDescriptorLocation = sdpFileString.find("m=audio");
    }

    LOG_DEBUG(subprocess) << "location:" << mediaDescriptorLocation;

    if ((mediaDescriptorLocation!=std::string::npos) && (rtpDescriptorLocation != std::string::npos))
    {
        size_t portLeftBound = mediaDescriptorLocation + sizeof("m=video") ; // m=audio and m=video have the same length 

        // replace the port with our outgoing port
        std::string newSdpFileString = std::string(&sdpFileString[0], &sdpFileString[portLeftBound]); // first half
        newSdpFileString.append(std::to_string(m_outgoingRtpPort) + " "); // add new port and space
        newSdpFileString.append(std::string(&sdpFileString[rtpDescriptorLocation], &sdpFileString[sdpFileString.size()])); // remaining half
        m_sdpFileString = newSdpFileString;
        LOG_INFO(subprocess) << "New SDP String:\n" << m_sdpFileString;
        return 0;
    }

    return -1;
}

int BpReceiveStream::ExecuteFFmpegInstance()
{
    std::string finalCommand;
    finalCommand = m_ffmpegCommand;

    std::string sdp = "\"";
    sdp.append(FFMPEG_SDP_HEADER);
    sdp.append(m_sdpFileString);
    sdp.pop_back(); // remove new line
    sdp.append("\"");

    size_t inputParamLocation = finalCommand.find("-i") + 3 ; // offest from -i 

    if (inputParamLocation != std::string::npos)
    {
        finalCommand.insert(inputParamLocation, sdp);
        LOG_INFO(subprocess) << "Final ffmpeg command: \n" << finalCommand;
        
        boost::process::child ffmpegProcess(finalCommand);
        ffmpegProcess.detach();
    }


    return 0;
}

bool BpReceiveStream::TryWaitForIncomingDataAvailable(const boost::posix_time::time_duration &timeout)
{
    if (m_incomingCircularPacketQueue.size() == 0) { // if empty, we wait
        return GetNextIncomingPacketTimeout(timeout);
    }
    return true; 
}

bool BpReceiveStream::GetNextIncomingPacketTimeout(const boost::posix_time::time_duration &timeout)
{
    boost::mutex::scoped_lock lock(m_incomingQueueMutex);
    if ((m_incomingCircularPacketQueue.size() == 0)) {
        m_incomingQueueCv.timed_wait(lock, timeout); //lock mutex (above) before checking condition
        return false;
    }
    
    return true;
}



bool BpReceiveStream::TryWaitForSuccessfulSend(const boost::posix_time::time_duration &timeout)
{
    if (!m_sentPacketsSuccess) { 
        return GetSuccessfulSendTimeout(timeout);
    }
    return true; 
}

bool BpReceiveStream::GetSuccessfulSendTimeout(const boost::posix_time::time_duration &timeout)
{
    boost::mutex::scoped_lock lock(m_sentPacketsMutex);
    if (!m_sentPacketsSuccess) {
        m_cvSentPacket.timed_wait(lock, timeout); //lock mutex (above) before checking condition
        return false;
    }
    
    return true;
}



void BpReceiveStream::OnSentRtpPacketCallback(bool success, std::shared_ptr<std::vector<UdpSendPacketInfo>> &udpSendPacketInfoVecSharedPtr, const std::size_t numPacketsSent)
{
    m_sentPacketsSuccess = true;
    m_cvSentPacket.notify_one();

    if (success) 
    {
        m_totalRtpPacketsSent += numPacketsSent;
        m_totalRtpBytesSent += udpSendPacketInfoVecSharedPtr->size();
        LOG_DEBUG(subprocess) << "Sent " <<  numPacketsSent << " packets. Sent " << udpSendPacketInfoVecSharedPtr->size() << " bytes";
    } else {
        LOG_ERROR(subprocess) << "Failed to send RTP packet";
    }
}


int BpReceiveStream::SendUdpPacket(const std::vector<uint8_t>& message) {


    m_totalRtpBytesSent += socket.send_to(boost::asio::buffer(message), m_udpEndpoint);
	// try {
	// } catch (const boost::system::system_error& ex) {
	// 	// Exception thrown!
	// 	// Examine ex.code() and ex.what() to see what went wrong!
    //     m_totalRtpPacketsFailedToSend++;
    //     LOG_ERROR(subprocess) << "Failed to send code: " << ex.code() << " what:" << ex.what();
	// 	return -1;
	// }

    m_totalRtpPacketsSent++;
	return 0;
}