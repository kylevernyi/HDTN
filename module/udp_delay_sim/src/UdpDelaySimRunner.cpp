/**
 * @file UdpDelaySimRunner.cpp
 * @author  Brian Tomko <brian.j.tomko@nasa.gov>
 *
 * @copyright Copyright © 2021 United States Government as represented by
 * the National Aeronautics and Space Administration.
 * No copyright is claimed in the United States under Title 17, U.S.Code.
 * All Other Rights Reserved.
 *
 * @section LICENSE
 * Released under the NASA Open Source Agreement (NOSA)
 * See LICENSE.md in the source root directory for more information.
 */

#include "UdpDelaySim.h"
#include "UdpDelaySimRunner.h"
#include "SignalHandler.h"
#include "Logger.h"

#include <fstream>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/date_time.hpp>
#include <boost/random.hpp>
#include "TimestampUtil.h"

static constexpr hdtn::Logger::SubProcess subprocess = hdtn::Logger::SubProcess::none;

void UdpDelaySimRunner::MonitorExitKeypressThreadFunction() {
    LOG_INFO(subprocess) << "Keyboard Interrupt.. exiting";
    m_runningFromSigHandler = false;
}

static bool DropPacket(const std::vector<uint8_t>& udpPacketReceived, boost::mt19937& prng, uint32_t dropIfBelowThisRandomNumber, bool neverDropLtpPing) {
    if (neverDropLtpPing) {
        const uint8_t ltpHeaderByte = udpPacketReceived[0];
        if (ltpHeaderByte == 12) { //CANCEL_SEGMENT_FROM_BLOCK_SENDER = 12
            return false; //don't drop pings ever
        }
    }
    return prng() < dropIfBelowThisRandomNumber;
}

UdpDelaySimRunner::UdpDelaySimRunner() : m_runningFromSigHandler(false) {}
UdpDelaySimRunner::~UdpDelaySimRunner() {}


bool UdpDelaySimRunner::Run(int argc, const char* const argv[], volatile bool & running, bool useSignalHandler) {
    //scope to ensure clean exit before return 0
    {
        running = true;
        m_runningFromSigHandler = true;
        SignalHandler sigHandler(boost::bind(&UdpDelaySimRunner::MonitorExitKeypressThreadFunction, this));
        
        std::string remoteUdpHostname;
        uint16_t remoteUdpPort;
        std::string remoteUdpPortAsString;
        uint16_t myBoundUdpPort;
        uint64_t sendDelayMs;
        uint64_t lossOfSignalStartMs;
        uint64_t lossOfSignalDurationMs;
        unsigned int numUdpRxPacketsCircularBufferSize;
        unsigned int maxRxUdpPacketSizeBytes;
        double packetDropRatePercentage;
        bool neverDropLtpPing = false;
        boost::mt19937 prng(static_cast<uint32_t>(TimestampUtil::GetSecondsSinceEpochRfc5050()));

        boost::program_options::options_description desc("Allowed options");
        try {
            desc.add_options()
                ("help", "Produce help message.")
                ("remote-udp-hostname", boost::program_options::value<std::string>()->default_value("localhost"), "Forwarding destination UDP hostname.")
                ("remote-udp-port", boost::program_options::value<uint16_t>()->default_value(1113), "Forwarding destination UDP port.")
                ("my-bound-udp-port", boost::program_options::value<uint16_t>()->default_value(1114), "My bound UDP port (to receive on).")
                ("num-rx-udp-packets-buffer-size", boost::program_options::value<unsigned int>()->default_value(100), "UDP max packets to receive (circular buffer size).")
                ("max-rx-udp-packet-size-bytes", boost::program_options::value<unsigned int>()->default_value(1500), "Maximum size (bytes) of a UDP packet to receive (1500 byte for small ethernet frames).")
                ("send-delay-ms", boost::program_options::value<uint64_t>()->default_value(1), "Delay in milliseconds before forwarding received udp packets.")
                ("los-start-ms", boost::program_options::value<uint64_t>()->default_value(0), "Delay in milliseconds after first RX udp packet before entering Loss of Signal (LOS) (0=disabled).")
                ("los-duration-ms", boost::program_options::value<uint64_t>()->default_value(0), "Duration of Loss of Signal (LOS).")
                ("packet-drop-rate-percentage", boost::program_options::value<double>()->default_value(0.), "Percentage of packets dropped.")
                ("never-drop-ltp-ping", "Never drop the ltp ping packets if packet drop rate is enabled. (default disabled)")
                ;

            boost::program_options::variables_map vm;
            boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc, boost::program_options::command_line_style::unix_style | boost::program_options::command_line_style::case_insensitive), vm);
            boost::program_options::notify(vm);

            if (vm.count("help")) {
                LOG_INFO(subprocess) << desc;
                return false;
            }

            remoteUdpHostname = vm["remote-udp-hostname"].as<std::string>();
            remoteUdpPort = vm["remote-udp-port"].as<boost::uint16_t>();
            remoteUdpPortAsString = boost::lexical_cast<std::string>(remoteUdpPort);
            myBoundUdpPort = vm["my-bound-udp-port"].as<boost::uint16_t>();
            sendDelayMs = vm["send-delay-ms"].as<uint64_t>();
            lossOfSignalStartMs = vm["los-start-ms"].as<uint64_t>();
            lossOfSignalDurationMs = vm["los-duration-ms"].as<uint64_t>();
            numUdpRxPacketsCircularBufferSize = vm["num-rx-udp-packets-buffer-size"].as<unsigned int>();
            maxRxUdpPacketSizeBytes = vm["max-rx-udp-packet-size-bytes"].as<unsigned int>();
            packetDropRatePercentage = vm["packet-drop-rate-percentage"].as<double>();

            if (vm.count("never-drop-ltp-ping")) {
                neverDropLtpPing = true;
            }
        }
        catch (boost::bad_any_cast & e) {
            LOG_ERROR(subprocess) << "invalid data error: " << e.what() << "\n";
            LOG_ERROR(subprocess) << desc;
            return false;
        }
        catch (std::exception& e) {
            LOG_ERROR(subprocess) << e.what();
            return false;
        }
        catch (...) {
            LOG_ERROR(subprocess) << "Exception of unknown type!";
            return false;
        }

        LOG_INFO(subprocess) << "starting UdpDelaySim (Proxy)..";
        UdpDelaySim udpDelaySim(myBoundUdpPort, remoteUdpHostname, remoteUdpPortAsString,
            numUdpRxPacketsCircularBufferSize, maxRxUdpPacketSizeBytes,
            boost::posix_time::milliseconds(sendDelayMs),
            lossOfSignalStartMs, lossOfSignalDurationMs, true);
        
        if (useSignalHandler) {
            sigHandler.Start(false);
        }
        if ((packetDropRatePercentage > 0.001) && (packetDropRatePercentage < 100.0)) {
            //prng.max() is 4294967295 or UINT32_MAX
            uint32_t threshold = static_cast<uint32_t>(packetDropRatePercentage * 0.01 * UINT32_MAX);
            LOG_DEBUG(subprocess) << "Dropping packets with loss rate of " << packetDropRatePercentage
                << " that fall below threshold " << threshold;
            udpDelaySim.SetUdpDropSimulatorFunction_ThreadSafe(boost::bind(&DropPacket, boost::placeholders::_1, boost::ref(prng), threshold, neverDropLtpPing));
        }
        LOG_INFO(subprocess) << "UdpDelaySim up and running";
        while (running && m_runningFromSigHandler) {
            boost::this_thread::sleep(boost::posix_time::millisec(250));
            if (useSignalHandler) {
                sigHandler.PollOnce();
            }
        }

    }
    LOG_INFO(subprocess) << "UdpDelaySim: exited cleanly";
    return true;
}
