
#include <string.h>

#include "../contract/ndp/NDPCommand_m.h"
#include "ndp_common/NdpHeader.h"
#include "Ndp.h"
#include "NDPAlgorithm.h"
#include "NDPConnection.h"
#include "NDPReceiveQueue.h"
#include "NDPSendQueue.h"


namespace inet {

namespace ndp {

//
// Event processing code
//
void NDPConnection::process_OPEN_ACTIVE(NDPEventCode& event, NDPCommand *ndpCommand, cMessage *msg)
{
    NDPOpenCommand *openCmd = check_and_cast<NDPOpenCommand *>(ndpCommand);
    L3Address localAddr, remoteAddr;
    int localPort, remotePort;

    switch (fsm.getState()) {
        case NDP_S_INIT:
            initConnection(openCmd);
//            std::cout << "\n\n\n NDPConnection::process_OPEN_ACTIVE \n";

            // store local/remote socket
            state->active = true;
            localAddr = openCmd->getLocalAddr();
            remoteAddr = openCmd->getRemoteAddr();
            localPort = openCmd->getLocalPort();
            remotePort = openCmd->getRemotePort();

            state->numPacketsToSend = openCmd->getNumPacketsToSend();
            state->isSender =  openCmd->isSender();  // true
            state->isReceiver = openCmd->isReceiver(); // false
            state->isLongFlow = openCmd->isLongFlow();
            state->priorityValue = openCmd->getPriorityValue();


//            std::cout<< "   state->fileId   "    << state->fileId <<std::endl;

            if (remoteAddr.isUnspecified() || remotePort == -1)
                throw cRuntimeError(ndpMain, "Error processing command OPEN_ACTIVE: remote address and port must be specified");

            if (localPort == -1) {
                localPort = ndpMain->getEphemeralPort();
                EV_DETAIL << "Assigned ephemeral port " << localPort << "\n";
            }

            EV_DETAIL << "\nprocess_OPEN_ACTIVE OPEN: " << localAddr << ":" << localPort << " --> " << remoteAddr << ":" << remotePort << "\n";

             ndpMain->addSockPair(this, localAddr, remoteAddr, localPort, remotePort);

             sendEstabIndicationToApp();
             //             sendSyn();
             sendQueue->init(state->numPacketsToSend , B(state->snd_mss)); //added B
//             generatePacketsList();
             sendInitialWindow();
//            startSynRexmitTimer(); // timer for SYN mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm
//            scheduleTimeout(connEstabTimer, NDP_TIMEOUT_CONN_ESTAB);  // connection establishing timer

            break;

        default:
            throw cRuntimeError(ndpMain, "Error processing command OPEN_ACTIVE: connection already exists");
    }

    delete openCmd;
    delete msg;
}

void NDPConnection::process_OPEN_PASSIVE(NDPEventCode& event, NDPCommand *ndpCommand, cMessage *msg)
{
    NDPOpenCommand *openCmd = check_and_cast<NDPOpenCommand *>(ndpCommand);
    L3Address localAddr;
    int localPort;

    switch (fsm.getState()) {
        case NDP_S_INIT:
            initConnection(openCmd);
            state->active = false;
             localAddr = openCmd->getLocalAddr();
            localPort = openCmd->getLocalPort();
            state->isSender =  openCmd->isSender(); // false
            state->isReceiver = openCmd->isReceiver(); // true

            if (localPort == -1)
                throw cRuntimeError(ndpMain, "Error processing command OPEN_PASSIVE: local port must be specified");

            EV_DETAIL << "Starting to listen on: " << localAddr << ":" << localPort << "\n";
//             std::cout << "NDPConnection::process_OPEN_PASSIVE Starting to listen on: " << localAddr << ":" << localPort << "\n";

            ndpMain->addSockPair(this, localAddr, L3Address(), localPort, -1);
            //ndpMain->addSockPair(this, localAddr, L3Address(), localPort, 69);
            break;

        default:
            throw cRuntimeError(ndpMain, "Error processing command OPEN_PASSIVE: connection already exists");
    }

    delete openCmd;
    delete msg;
}

void NDPConnection::process_OPTIONS(NDPEventCode& event, NDPCommand *ndpCommand, cMessage *msg)
{
    ASSERT(event == NDP_E_SETOPTION);

    if (auto cmd = dynamic_cast<NDPSetTimeToLiveCommand *>(ndpCommand))
        ttl = cmd->getTtl();
//    else if (auto cmd = dynamic_cast<NDPSetTosCommand *>(ndpCommand)) {
//        tos = cmd->getTos();
//    }
//    else if (auto cmd = dynamic_cast<NDPSetDscpCommand *>(ndpCommand)) {
//        dscp = cmd->getDscp();
//    }
    else
        throw cRuntimeError("Unknown subclass of NDPSetOptionCommand received from app: %s", ndpCommand->getClassName());
    delete ndpCommand;
    delete msg;
}


} // namespace ndp

} // namespace inet

