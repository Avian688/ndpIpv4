#include "GenericAppMsgNdp_m.h"
#include "NdpBasicClientApp.h"
#include "inet/common/lifecycle/ModuleOperations.h"
#include "inet/common/ModuleAccess.h"
#include "inet/common/TimeTag_m.h"
#include <iostream>
#include <random>

namespace inet {

#define MSGKIND_CONNECT    0
#define MSGKIND_SEND       1

Define_Module(NdpBasicClientApp);
//simsignal_t NdpBasicClientApp::packetReceivedSignal =  registerSignal("packetReceived");

NdpBasicClientApp::~NdpBasicClientApp() {
    cancelAndDelete(timeoutMsg);

    // Added
//    if (requestRexmitTimer)
//        delete cancelEvent(requestRexmitTimer);
}

void NdpBasicClientApp::initialize(int stage) {
    NdpAppBase::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        bytesRcvd = 0;
        packetsRcvd=0;
        WATCH(bytesRcvd);
        sendBytes = par("sendBytes");   // commented moh

        startTime = par("startTime");
        stopTime = par("stopTime");
        if (stopTime >= SIMTIME_ZERO && stopTime < startTime)
            throw cRuntimeError("Invalid startTime/stopTime parameters");
        timeoutMsg = new cMessage("timer");
    }
}

void NdpBasicClientApp::handleStartOperation(LifecycleOperation *operation)
{
    simtime_t now = simTime();
    simtime_t start = std::max(startTime, now);
    if (timeoutMsg && ((stopTime < SIMTIME_ZERO) || (start < stopTime) || (start == stopTime && startTime == stopTime))) {
        timeoutMsg->setKind(MSGKIND_CONNECT);
        scheduleAt(start, timeoutMsg);
    }
}

void NdpBasicClientApp::handleStopOperation(LifecycleOperation *operation)
{
    cancelEvent(timeoutMsg);
    if (socket.getState() == NDPSocket::CONNECTED || socket.getState() == NDPSocket::CONNECTING || socket.getState() == NDPSocket::PEER_CLOSED)
        close();
}

void NdpBasicClientApp::handleCrashOperation(LifecycleOperation *operation)
{
    cancelEvent(timeoutMsg);
    if (operation->getRootModule() != getContainingNode(this))
        socket.destroy();
}

void NdpBasicClientApp::sendWriteRequest() {

    long requestLength = par("requestLength");
    long replyLength = par("replyLength");

    if (requestLength < 1)
        requestLength = 1;
    if (replyLength < 1)
        replyLength = 1;

    char msgname[16];
//    sprintf(msgname, "REQUEST-%d",++requestIndex);
    sprintf(msgname, "REQUEST-WRITE-INIT");

    const auto& payload = makeShared<GenericAppMsgNdp>();
    Packet *packet = new Packet("REQUEST-WRITE-INIT");
    payload->setChunkLength(B(requestLength));
    //payload->setExpectedReplyLength(B(replyLength));
    payload->setServerClose(false);
    payload->addTag<CreationTimeTag>()->setCreationTime(simTime());
    packet->insertAtBack(payload);
    sendPacket(packet);
}

void NdpBasicClientApp::handleTimer(cMessage *msg) {
    /////// Added MOH send requests based on a timer
    switch (msg->getKind()) {

    case MSGKIND_CONNECT:
        connect();    // active OPEN
        break;

    case MSGKIND_SEND:   // not used now MHO see  NdpBasicClientApp::socketEstablished
//        sendRequest();
        break;

    default:
        throw cRuntimeError("Invalid timer msg: kind=%d", msg->getKind());
    }
}

void NdpBasicClientApp::socketEstablished(NDPSocket *socket) {
    NdpAppBase::socketEstablished(socket);
//    sendData();
//      if (opcode == 2)  sendWriteRequest();   // 1 read, 2 write
}

void NdpBasicClientApp::rescheduleOrDeleteTimer(simtime_t d,
        short int msgKind) {
    cancelEvent(timeoutMsg);

    if (stopTime < SIMTIME_ZERO|| d < stopTime) {
        timeoutMsg->setKind(msgKind);
        scheduleAt(d, timeoutMsg);
    }
    else {
        delete timeoutMsg;
        timeoutMsg = nullptr;
    }
}

void NdpBasicClientApp::socketDataArrived(NDPSocket *socket, Packet *msg, bool urgent) {
//    if (msg->getKind() == NDP_I_DATA) {
//        std::cout << "reply arrived to the client  " << std::endl;
//        packetsRcvd++;
//        cPacket *pk = PK(msg);
//        long packetLength = pk->getByteLength();
//        bytesRcvd += packetLength;
//        emit(rcvdSymbolSignal, pk);
//         std::cout << "NdpBasicClientApp::socketDataArrived , packetsRcvd  " << packetsRcvd << std::endl;
////        delete msg;
//    }

 }

void NdpBasicClientApp::close()
{
    NdpAppBase::close();
    cancelEvent(timeoutMsg);
}
void NdpBasicClientApp::socketClosed(NDPSocket *socket) {
    NdpAppBase::socketClosed(socket);
}

void NdpBasicClientApp::socketFailure(NDPSocket *socket, int code) {
    NdpAppBase::socketFailure(socket, code);

    // reconnect after a delay
    if (timeoutMsg) {
        simtime_t d = simTime() + (simtime_t) par("reconnectInterval");
        rescheduleOrDeleteTimer(d, MSGKIND_CONNECT);
    }
}


}// namespace inet

