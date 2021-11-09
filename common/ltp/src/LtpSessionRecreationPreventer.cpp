#include "LtpSessionRecreationPreventer.h"
#include <iostream>

LtpSessionRecreationPreventer::LtpSessionRecreationPreventer(const uint64_t numReceivedSessionsToRemember) :
    M_NUM_RECEIVED_SESSION_NUMBERS_TO_REMEMBER(numReceivedSessionsToRemember),
    m_previouslyReceivedSessionNumbersUnorderedSet(numReceivedSessionsToRemember + 10), //intial num buckets to prevent rehash
    m_previouslyReceivedSessionNumbersQueueVector(numReceivedSessionsToRemember),
    m_queueIsFull(false),
    m_nextQueueIndex(0)
{

}

LtpSessionRecreationPreventer::~LtpSessionRecreationPreventer() {}

bool LtpSessionRecreationPreventer::AddSession(const uint64_t newSessionNumber) {
    if (m_previouslyReceivedSessionNumbersUnorderedSet.insert(newSessionNumber).second) { //successful insertion
        if (m_queueIsFull) { //remove oldest session number from history
            if (m_previouslyReceivedSessionNumbersUnorderedSet.erase(m_previouslyReceivedSessionNumbersQueueVector[m_nextQueueIndex]) == 0) {
                std::cerr << "error in LtpSessionRecreationPreventer::AddSession: unable to erase an old value\n";
                return false;
            }
        }
        m_previouslyReceivedSessionNumbersQueueVector[m_nextQueueIndex++] = newSessionNumber;

        if (m_nextQueueIndex >= M_NUM_RECEIVED_SESSION_NUMBERS_TO_REMEMBER) {
            m_nextQueueIndex = 0;
            m_queueIsFull = true;
        }
        return true;
    }
    return false;
}
bool LtpSessionRecreationPreventer::ContainsSession(const uint64_t newSessionNumber) const {
    return (m_previouslyReceivedSessionNumbersUnorderedSet.count(newSessionNumber) != 0);
}

