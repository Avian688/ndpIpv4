#include <string.h>

#include "ndp_common/NdpHeader.h"
#include "Ndp.h"
#include "NDPAlgorithm.h"
#include "NDPReceiveQueue.h"
#include "NDPSendQueue.h"
#include "NDPConnection.h"
#include "../contract/ndp/NDPCommand_m.h"
#include "../../application/ndpapp/GenericAppMsgNdp_m.h"
#include "inet/applications/common/SocketTag_m.h"
#include "ndp_common/NdpHeader.h"
//Preprocessor directives
//  #define  ShowOut
 #ifdef ShowOut
#define MY_COUT std::cout
#else
#define MY_COUT if(false) std::cout
#endif

namespace inet {
namespace ndp {

void NDPConnection::generatePacketsList() {

}

// to be removed
void NDPConnection::addNewPacketToSend() {
 }




//MOH
void NDPConnection::sendInitialWindow() {

    // TODO  we don't do any checking about the received request segment, e.g. check if it's  a request nothing else
    // fetch the next Packet from the encodingPackets list
    std::list<PacketsToSend>::iterator itt;

    if (state->isLongFlow == true)
        state->IW = 5;

//    if (state->sequenceNumber == 0) {
//        MY_COUT << " ££££££££££££  Sender ££££££££££££££££££££ sendInitialWindow "<< std::endl;

        if (state->IW > state->numPacketsToSend)
            state->IW = state->numPacketsToSend;
        //state->IW = 8;
        for (int i = 1; i <= state->IW; i++) {
//            MY_COUT << "  sendInitialWindow "   << std::endl;
//            itt = sentPakcetsList.begin();
//            std::advance(itt, state->sequenceNumber); // increment the iterator by esi

            // create a segment for the Packet that will be sent
            //NdpHeader *ndpseg = new NdpHeader();
            //const auto& ndpseg = makeShared<NdpHeader>();
//            ++state->sequenceNumber;
            std::tuple<Ptr<NdpHeader>, Packet*> packSeg = sendQueue->getNdpHeader();
            auto ndpseg = std::get<0>(packSeg);
            auto fp = std::get<1>(packSeg);
            if (ndpseg) {
                ndpseg->setIsDataPacket(true);
                ndpseg->setIsPullPacket(false);
                ndpseg->setIsHeader(false);
                ndpseg->setSynBit(true);  //  oooooooooooooooooooooooooo
                ndpseg->setAckBit(false);
                ndpseg->setNackBit(false);
                ndpseg->setIsLongFlow(state->isLongFlow);

                ndpseg->setNumPacketsToSend(state->numPacketsToSend);
                ndpseg->setPriorityValue(state->priorityValue);
//                ndpseg->setIsLastPacketToSend(false);
//                ndpseg->setDataSequenceNumber(state->sequenceNumber);
                //Packet *fp = new Packet("Init-Win Packet");
                sendToIP(fp, ndpseg);
            }
        }


}


NDPEventCode NDPConnection::process_RCV_SEGMENT(Packet *packet, const Ptr<const NdpHeader>& ndpseg, L3Address src, L3Address dest) {
    EV_INFO << "Seg arrived: ";
    printSegmentBrief(packet, ndpseg);
    EV_DETAIL << "TCB: " << state->str() << "\n";

    NDPEventCode event;
    if (fsm.getState() == NDP_S_LISTEN) {
         event = processSegmentInListen(packet, ndpseg, src, dest);
         if (event == NDP_E_RCV_SYN) {
             FSM_Goto(fsm, NDP_S_ESTABLISHED);
             event = processSegment1stThru8th(packet, ndpseg);
         }
    }
    else {
        ndpMain->updateSockPair(this, dest, src,  ndpseg->getDestPort(), ndpseg->getSrcPort());
        event = processSegment1stThru8th(packet, ndpseg);
    }
    delete packet;
    return event;
}



void NDPConnection::ackArrivedFreeBuffer(Packet* packet, unsigned int acknum) {

}

void NDPConnection::getFirstBufferedPkt( std::list<PacketsToSend>::iterator& iter ) {

}

void NDPConnection::getBufferedPkt(unsigned int seqNum , std::list<PacketsToSend>::iterator& iter ) {

}

NDPEventCode NDPConnection::processSegment1stThru8th(Packet *packet, const Ptr<const NdpHeader>& ndpseg) {

     NDPEventCode event = NDP_E_IGNORE;
    if (ndpseg->getFinBit()) {

         event = NDP_E_RCV_FIN;

        switch (fsm.getState()) {
        case NDP_S_FIN_WAIT_1:

            break;
        default:
            break;
        }
        return event;
    }


    if (fsm.getState() == NDP_S_LAST_ACK) {
        return NDP_E_RCV_ACK;
    }


    bool isTargetReq = false;
    //if(getLocalAddr().str() == "10.0.0.177" && ndpseg->getDestPort() == 1026){
    //    std::cout << "\nSENDER";
    //    isTargetReq = true;
    //}
    // (S.1)   at the sender: NACK Arrived at the sender, then prepare the trimmed pkt for retranmission
    //        (not to transmit yet just make it to be the first one to transmit upon getting a pull pkt later)
    // ££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££
    // ££££££££££££££££££££££££ NACK Arrived at the sender £££££££££££££££££££ Tx
    // ££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££
    if (fsm.getState() == NDP_S_ESTABLISHED && state->isSender == true && ndpseg->getNackBit() == true) {
//        MY_COUT  << "  \n\n\n  ££££££££££££££££  Sender   ££££££££££££££ new  ACK arrived: ackNum = " << ndpseg->getNackNo() << std::endl;
//        ackArrivedFreeBuffer(ndpseg->getNackNo());
        sendQueue->nackArrivedMoveFront(ndpseg->getNackNo());
        // TODO
        //if(getLocalAddr().str() == "10.0.0.177" && ndpseg->getDestPort() == 1026){
        //    std::cout << "\nNACK ARRIVED FOR SEQ NUM: " << ndpseg->getNackNo();
        //    isTargetReq = true;
        //}
    }

    // (S.2)  at the sender:  ACK arrived, so free the acknowledged pkt from the buffer.
    // ££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££
    // ££££££££££££££££££££££££ ACK Arrived at the sender £££££££££££££££££££ Tx
    // ££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££
    if (fsm.getState() == NDP_S_ESTABLISHED &&  state->isSender == true && ndpseg->getAckBit() == true) {
//        MY_COUT  << "  \n\n\n  £££££££££ Sender ££££££££ new  ACK arrived: ackNum = "  << ndpseg->getAckNo() << std::endl;
        sendQueue->ackArrivedFreeBuffer(ndpseg->getAckNo());
    }

    // (S.3)  at the sender: PULL pkt arrived, this pkt triggers either retransmission of trimmed pkt or sending a new data pkt.
// ££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££
// ££££££££££££££££££££££££ REQUEST Arrived at the sender £££££££££££££££
// ££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££££
   if (fsm.getState() == NDP_S_ESTABLISHED && state->isSender == true &&  ndpseg->isPullPacket() == true) {

//          MY_COUT << "  \n\n\n  ££££££££££££££££  Sender ££££££££££££££ new  REQUEST arrived: RequestSequenceNumber= " << ndpseg->getPullSequenceNumber() << std::endl;
//        MY_COUT << " sender " << getNDPMain()->getFullPath()  << "\n";
//        MY_COUT << " "<< cSimulation::getActiveSimulation()->getContextSimpleModule()->getFullPath() << std::endl;


        if(getLocalAddr().str() == "10.0.0.177" && ndpseg->getDestPort() == 1026){
            std::cout << "\nPULL ARRIVED " << ndpseg->getPullSequenceNumber();
            isTargetReq = true;
        }
        int requestsGap = ndpseg->getPullSequenceNumber() - state->internal_request_id;
        EV_INFO << "\n\n\n\n\nPULL SEQ NUMB: "<< ndpseg->getPullSequenceNumber() << "\n";
        EV_INFO << "\nINTERNAL REQ NUMB: "<< state->internal_request_id << "\n";
        EV_INFO << "\nREQUESTGAP: "<< requestsGap << "\n\n\n\n\n";
//            MY_COUT <<  "  requestsGap=  " << requestsGap << std::endl;

        if (requestsGap >= 1) {
            //  we send Packets  based on requestsGap value
            // if the requestsGap is smaller than 1 that means we received a delayed request which we need to  ignore
            // as we have assumed it was lost and we send extra Packets previously
            for (int i = 1; i <= requestsGap; i++) {
                ++state->internal_request_id;
                std::tuple<Ptr<NdpHeader>, Packet*> headPack = sendQueue->getNdpHeader();
                const auto& ndpseg = std::get<0>(headPack);
                Packet *fp = std::get<1>(headPack);
                if (ndpseg) {
                    ndpseg->setIsDataPacket(true);
                    ndpseg->setIsPullPacket(false);
                    ndpseg->setIsHeader(false);

                    ndpseg->setAckBit(false);
                    ndpseg->setNackBit(false);
                    ndpseg->setSynBit(false);

                    ndpseg->setIsLongFlow(state->isLongFlow);
                    ndpseg->setNumPacketsToSend(state->numPacketsToSend);
                    ndpseg->setPriorityValue(state->priorityValue);
                    //ndpseg->setDataSequenceNumber(state->snd_nxt); //maybe change
//                    ndpseg->setIsLastPacketsToSend(false);
//                    ndpseg->setDataSequenceNumber(state->sequenceNumber);
                    //Packet *fp = new Packet("Ndp");
                    //if(isTargetReq){
                    //    std::cout << "\nDATA SEQ NUM " << ndpseg->getDataSequenceNumber() << " BEING SENT TO RECEIVER\n\n";
                    //}
                    sendToIP(fp, ndpseg);
                }
                //else{
                //    --state->internal_request_id;
                //}
            }
        } else if (requestsGap < 1) {
            EV_INFO << "Delayed pull arrived --> ignore it \n";
        }
    }

// (R.1)  at the receiver
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$  HEADER arrived   $$$$$$$$$$$$$$$$   Rx
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// header arrived at the receiver==> send new request with pacing (fixed pacing: MTU/1Gbps)
   //if(getRemoteAddr().str() == "10.0.0.125" && ndpseg->getDestPort() == 82){
   //        //std::cout <<  "\nport: " << ndpseg->getDestPort();
   //        std::cout << "\nRECEIVER";
   //       isTargetReq = true;
   //    }

    if (fsm.getState() == NDP_S_ESTABLISHED   && ndpseg->isHeader() == true   && ndpseg->isDataPacket() == false ) { // 1 read, 2 write

//        MY_COUT  << "  $$$$$$$$$$$$$$$  Receiver $$$$$$$$$$$$$$$$$  HEADER arrived.  " << std::endl;
//         MY_COUT  << " \n\n\n\n\n $$$$$$$$$$$$$$$  Receiver $$$$$$$$$$$$$$$$$  HEADER arrived.  " <<  getNDPMain()->getFullPath() << std::endl;
        sendNackNdp(ndpseg->getDataSequenceNumber());
      //  if(getRemoteAddr().str() == "10.0.0.125" && ndpseg->getDestPort() == 82){
      //      std::cout << "\nNACK BEING SENT FOR " << ndpseg->getDataSequenceNumber();
      //      isTargetReq = true;
       // }

        if (state->isLongFlow == false) {
            ++state->numRcvTrimmedHeader;
            addRequestToPullsQueue();
        }
        if (state->numberReceivedPackets == 0 && state->connNotAddedYet == true) {
            getNDPMain()->requestCONNMap[getNDPMain()->connIndex] = this; // moh added

            state->connNotAddedYet = false;
            EV << "sendFirstRequest sendFirstRequest sendFirstRequest sendFirstRequest .\n";

            ++getNDPMain()->connIndex;
            getNDPMain()->sendFirstRequest();
        }
    }

// (R.2) at the receiver
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$  data pkt arrived at the receiver  $$$$$$$$$$$$$$$$
// $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

    if (fsm.getState() == NDP_S_ESTABLISHED &&   state->isReceiver == true && ndpseg->isDataPacket() == true && ndpseg->isHeader() == false) {
//        EV_INFO  << " \n\n\n\n $$$$$$$$$$$$$$$  Receiver $$$$$$$$$$$$$$$$$  packet arrived. SQN= " << ndpseg->getDataSequenceNumber() << std::endl;
        unsigned int arrivedPktSeqNo = ndpseg->getDataSequenceNumber();
        sendAckNdp(arrivedPktSeqNo);

//        ++state->sequenceNumber;

        bool isLongFlowPacket = ndpseg->isLongFlow();
        unsigned int seqNo = ndpseg->getDataSequenceNumber();


        ++state->numberReceivedPackets;
        unsigned int numberReceivedPackets = state->numberReceivedPackets;
        int initialSentPackets = state->IW;
        int wantedPackets = state->numPacketsToGet;

//        MY_COUT << " mmmmmmmmmmmmmm  numberReceivedPackets= " << numberReceivedPackets << std::endl;


          // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          // %%%%%%%%%%%%%%%%%%%%%%%% 1   %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
          // if long flow then just add a request and delete the received packet

          if (isLongFlowPacket == true) {
//              std::cout << " hiii \n\n\n";
              addRequestToPullsQueue();
              Ptr<Chunk> msgRx;
              msgRx = packet->removeAll();
              uint32 endSeqNo;
              //msgRx = check_and_cast<Packet *>(ndpseg->removeFirstPayloadMessage(endSeqNo)); // each segment has just one packet
              //msgRx = check_and_cast<Packet *>(packet->removeAtFront(b(endSeqNo), 0));
              //msgRx->eraseAll();
              //delete packet;
              //cMessage *msgRcvd = nullptr;
              //msgRcvd = check_and_cast<cMessage *>(msgRx);
              //delete msgRcvd;
              goto ll;
          }

          // numberReceivedPackets > wantedPackets
        if ( numberReceivedPackets >  wantedPackets &&  isLongFlowPacket == false) {
            state->connFinished = true;
            getNDPMain()->allConnFinished();
            goto ll;
        }

        // numberReceivedPackets < wantedPackets
//        EV_INFO << "\n\n\n\n\n\n\nNUMB RECEIVED PACKETS " << numberReceivedPackets << "\n";
//        EV_INFO << "\nNUMB WANTED PACKETS " << wantedPackets << "\n";
//        EV_INFO << "\nINITIAL SENT PACKETS " << initialSentPackets << "\n";
//        EV_INFO << "\nVALUE " << wantedPackets-initialSentPackets << "\n";
//        EV_INFO << "\nCONN FINISHED? " << state->connFinished << "\n";
//        EV_INFO << "\nIS LONG FLOW? " << isLongFlowPacket << "\n";
        //if (numberReceivedPackets <= (wantedPackets-initialSentPackets)  && state->connFinished == false && isLongFlowPacket == false) {
        if (numberReceivedPackets <= (wantedPackets-initialSentPackets)  && state->connFinished == false && isLongFlowPacket == false) {
            addRequestToPullsQueue();
        }


        if (numberReceivedPackets == 1 && state->connNotAddedYet == true ) {
            getNDPMain()->requestCONNMap[getNDPMain()->connIndex] = this; // moh added

            ++getNDPMain()->connIndex;
            state->connNotAddedYet = false;
            EV << "sendFirstRequest. \n";
            getNDPMain()->sendFirstRequest();
        }


        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        // %%%%%%%%%%%%%%%%%%%%%%%%%  4  %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        // %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
        //  send any received Packet to the app
        Ptr<Chunk> msgRx;
       // uint32 endSeqNo;
        //msgRx = packet->removeAtFront(b(endSeqNo),0);
        msgRx = packet->removeAll(); // each segment has just one packet
//        MY_COUT << " endSeqNo " << endSeqNo << "\n";

        if (state->connFinished == false && isLongFlowPacket == false) {
            // buffer the received Packet segment
            std::list<PacketsToSend>::iterator itR;  // received iterator
            itR = receivedPacketsList.begin();
            std::advance(itR, seqNo); // increment the iterator by esi
            // MOH: Send any received Packet to the app, just for now to test the Incast example, this shouldn't be the normal case
            //cMessage *msgRcvd = nullptr;
            //msgRcvd = check_and_cast<Packet *>(msgRx);
            Packet *newPacket = new Packet("ReceivedPacket", msgRx);
            PacketsToSend receivedPkts;
            receivedPkts.pktId = seqNo;
            receivedPkts.msg = newPacket;
            receivedPacketsList.push_back(receivedPkts);
            newPacket->setKind(NDP_I_DATA); // TBD currently we never send TCP_I_URGENT_DATA
            //NDPCommand *cmd = new NDPCommand();
            //cmd->setConnId(socketId);
            newPacket->addTag<SocketInd>()->setSocketId(socketId);
            //newPacket->setControlInfo(cmd);
            sendToApp(newPacket);
        }


        // All the Packets have been received
         if (state->isfinalReceivedPrintedOut == false && isLongFlowPacket == false) {
//             MY_COUT << "  multiSourcingGroupIndex = " << state->multiSourcingGroupIndex << "\n";
            EV_INFO << "\n\n\n\n\nRECEIVED PACKETS: "<< numberReceivedPackets << "\n\n\n\n\n";
            EV_INFO << "\n\n\n\n\nWANTED PACKETS: "<< wantedPackets << "\n\n\n\n\n";
            if (numberReceivedPackets == wantedPackets || state->connFinished == true) {
//            MY_COUT << " \n\n all Packets  received, no more request to be sent \n ";
                // checking that all the Packets  have been stored in the receivedPacketsList
//            MY_COUT << " aaaaaaaaaaaaa  All the Packets  have been received All the Packets  have been received All the Packets  have been received aaaaaaaaaaaaa " << receivedPacketsList.size()<< "\n";
                std::list<PacketsToSend>::iterator iter;  // received iterator
                iter = receivedPacketsList.begin();
                int index = 0;
                while (iter != receivedPacketsList.end()) {
                    //MY_COUT << ++index << "  SQN: " << iter->pktId << " msgName " << iter->msg->getFullName() << "\n";
                    iter++;
                }

                MY_COUT << " numRcvTrimmedHeader:    " <<   state->numRcvTrimmedHeader << "\n\n";
//             MY_COUT << " aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa sendIndicationToAppsendIndicationToApp\n\n\n";
                EV_INFO << "\n\n\n\nCONNECTION FINISHED!" <<  "\n\n\n";
                sendIndicationToApp(NDP_I_PEER_CLOSED); // this is ok if the sinkApp is used by one conn
                // getNDPMain()->allConnFinished();
                state->isfinalReceivedPrintedOut = true;
            }
         }
    }
    ll:
    return event;
}


void NDPConnection::addRequestToPullsQueue(){
    EV_INFO << "\n\n\n\n Add new request packet to pullsQueue.\n";

    ++state->request_id;
//     MY_COUT << "Add new request packet to pullsQueue.  ,   request_id = " << state->request_id << " \n";
    char msgname[16];
    sprintf(msgname, "PULL-%d", state->request_id);
    //NdpHeader *ndpseg = createNDPSegment(msgname);
    //Packet *ndppack = new Packet(msgname, 0);
    Packet *ndppack = new Packet(msgname);

    const auto& ndpseg = makeShared<NdpHeader>();
    //ndppack->setByteLength(10); //maybe bit?
    //ndpseg->setPayloadLength(10);
    //ndpseg->setHeaderLength(B(30));


    ndpseg->setIsDataPacket(false);
    ndpseg->setIsPullPacket(true);
    ndpseg->setIsHeader(false);
    ndpseg->setSynBit(false);  //  oooooooooooooooooooooooooo
    ndpseg->setAckBit(false);  //  oooooooooooooooooooooooooo
    ndpseg->setNackBit(false);   //oooooooooooooooooooooooooo

    ndpseg->setIsLongFlow(state->isLongFlow);
    ndpseg->setPriorityValue(state->priorityValue);

//    ndpseg->setIsLastPktToSend(false);


    ndpseg->setPullSequenceNumber(state->request_id);
    ndppack->insertAtFront(ndpseg);
     //    ndpseg->addPayloadMessage(msg->dup(),10);
    //sendToIP(ndpseg);
    //pullQueue.insert(ndppack);
    //pullQueue.insert(ndpseg);
    //pullQueue.insert(std::make_tuple(ndppack, ndpseg));
    pullQueue.insert(ndppack);
    //EV << "Adding new request to the pull queue -- pullsQueue length now = " << pullQueue.getLength()<< "\n\n\n\n";
    EV << "Adding new request to the pull queue -- pullsQueue length now = " << pullQueue.getLength()<< "\n\n\n\n";
//    MY_COUT << "pullsQueue length now = " << pullQueue.getLength()<< ".\n";
//    MY_COUT  << " mmmm  addRequestToPullsQueue  this->getFullPath() " << ndpMain->getFullPath() << " , len= " << pullQueue.getLength()   << "\n";
    bool napState = getNDPMain()->getNapState();
    //std::cout << "\n" << napState;
    if (napState == true) {
        getNDPMain()->requestTimer();

    }


}

void NDPConnection::sendRequestFromPullsQueue(){
//    MY_COUT  << " ssssssssssssss sendRequestFromPullsQueuemmmm  this->getFullPath() " << ndpMain->getFullPath()     << "\n";

    if (pullQueue.getLength() > 0 ){
        //Packet* fp = (Packet *) pullQueue.pop();
        Packet* fp = check_and_cast<Packet *>(pullQueue.pop());
        //const auto& ndpseg = fp->popAtFront<ndp::NdpHeader>();
        auto ndpseg = fp->removeAtFront<ndp::NdpHeader>();
        //ndpseg->setHeaderLength(B(1500));
        //ndpseg->setChunkLength(B(1));
        //ndpseg = dynamic_cast<IntrusivePtr<NdpHeader>>(ndpseg);
        //pullQueue.pop();
        EV << "a request has been popped from the Pull queue, the new queue length  = " << pullQueue.getLength()<< " \n\n";
        sendToIP(fp, ndpseg);
    }
}

int NDPConnection::getPullsQueueLength()  {
//    MY_COUT << "pullsQueue length now = " <<" " << pullQueue.getLength()<< ".\n";
    int len = pullQueue.getLength();
    //int len = pullQueue.getLength();
    return len;
}


// MOH added NOT used now
void NDPConnection::startRequestRexmitTimer() {

    state->request_rexmit_count = 0;
    state->request_rexmit_timeout = NDP_TIMEOUT_PULL_REXMIT;  // 0.5 sec

    if (requestInternalTimer->isScheduled())
        cancelEvent(requestInternalTimer);

    scheduleTimeout(requestInternalTimer, state->request_rexmit_timeout); // start the timer for Request segment
}


// MOH
void NDPConnection::process_TIMEOUT_REQUEST_REXMIT() {

}



bool NDPConnection::isConnFinished() {
    return state->connFinished;
}

int NDPConnection::getNumRcvdPackets() {
    return state->numberReceivedPackets ;
}



void NDPConnection::setConnFinished() {
    MY_COUT << "allReceivedCloseAllMultiSourcingConnsmmmm \n\n\n\n" ;

      state->connFinished = true;
}





NDPEventCode NDPConnection::processSegmentInListen(Packet *packet, const Ptr<const NdpHeader>& ndpseg,  L3Address srcAddr, L3Address destAddr) {
    EV_DETAIL << "Processing segment in LISTEN\n";
//    MY_COUT << "Processing segment in LISTEN\n";
    EV_INFO << " \n\n aaaMMM srcAddr" << srcAddr << " \n";
    EV_INFO << " \n\n aaaMMM destAddr" << destAddr << " \n";

    if (ndpseg->getSynBit()) {
        EV_DETAIL << "\n\n MMMM  SYN bit set: filling in foreign socket\n";
//        if (state->fork) {
//            NDPConnection *conn = cloneListeningConnection();    // "conn" is the clone which will handle the new connection, while "this" stay LISTENing
//            ndpMain->addForkedConnection(this, conn, destAddr, srcAddr, ndpseg->getDestPort(), ndpseg->getSrcPort());
//            EV_DETAIL << "Connection forked: new connection got new socketId=" << conn->socketId << ", "
//                                                                                           "old connection keeps LISTENing with socketId=" << socketId << "\n";
//            selectInitialSeqNum();
//            if (ndpseg->getHeaderLength() > NDP_MIN_HEADER_LENGTH) // Header options present? NDP_HEADER_OCTETS = 20
//                readHeaderOptions(ndpseg);
//            // this is a receiver
//            state->numPacketsToGet = ndpseg->getNumPacketsToSend();
//            conn->performStateTransition(NDP_E_RCV_SYN);
//
//            return NDP_E_IGNORE;
//        }
//        else{
            ndpMain->updateSockPair(this, destAddr, srcAddr,  ndpseg->getDestPort(), ndpseg->getSrcPort());

            selectInitialSeqNum();


            if (ndpseg->getHeaderLength() > NDP_MIN_HEADER_LENGTH) // Header options present? NDP_HEADER_OCTETS = 20
                readHeaderOptions(ndpseg);

            // this is a receiver
            state->numPacketsToGet = ndpseg->getNumPacketsToSend();
            return NDP_E_RCV_SYN; // this will take us to SYN_RCVD
//        }
    }


    EV_WARN << "Unexpected segment: dropping it\n";
    return NDP_E_IGNORE;
}




void NDPConnection::process_TIMEOUT_CONN_ESTAB() {
    switch (fsm.getState()) {
    case NDP_S_SYN_RCVD:
    case NDP_S_SYN_SENT:
// Nothing to do here. TIMEOUT_CONN_ESTAB event will automatically
// take the connection to LISTEN or CLOSED, and cancel SYN-REXMIT timer.
        if (state->active) {
            // notify user if we're on the active side
            sendIndicationToApp(NDP_I_TIMED_OUT);
        }
        break;

    default:
// We should not receive this timeout in this state.
        throw cRuntimeError(ndpMain,
                "Internal error: received CONN_ESTAB timeout in state %s",
                stateName(fsm.getState()));
    }
}

void NDPConnection::process_TIMEOUT_2MSL() {
    //"
    // If the time-wait timeout expires on a connection delete the TCB,
    // enter the CLOSED state and return.
    //"
    switch (fsm.getState()) {
    case NDP_S_TIME_WAIT:
        break;

    default:
// We should not receive this timeout in this state.
        throw cRuntimeError(ndpMain,
                "Internal error: received time-wait (2MSL) timeout in state %s",
                stateName(fsm.getState()));
    }
}

void NDPConnection::process_TIMEOUT_FIN_WAIT_2() {
    switch (fsm.getState()) {
    case NDP_S_FIN_WAIT_2:
// Nothing to do here. The TIMEOUT_FIN_WAIT_2 event will automatically take
// the connection to CLOSED.
        break;

    default:
// We should not receive this timeout in this state.
        throw cRuntimeError(ndpMain,
                "Internal error: received FIN_WAIT_2 timeout in state %s",
                stateName(fsm.getState()));
    }
}



void NDPConnection::segmentArrivalWhileClosed(Packet *packet, const Ptr<const NdpHeader>& ndpseg, L3Address srcAddr, L3Address destAddr) {
    EV_INFO << "Seg arrived: ";
    printSegmentBrief(packet, ndpseg);

    // This segment doesn't belong to any connection, so this object
    // must be a temp object created solely for the purpose of calling us

    ASSERT(state == nullptr);

    EV_INFO << "Segment doesn't belong to any existing connection\n";

    if (ndpseg->getRstBit()) {
        EV_DETAIL << "RST bit set: dropping segment\n";
        return;
    }

    if (!ndpseg->getAckBit()) {
        EV_DETAIL << "ACK bit not set: sending RST+ACK\n";
        uint32 ackNo = ndpseg->getDataSequenceNumber() + packet->getByteLength();
        sendRstAck(0, ackNo, destAddr, srcAddr, ndpseg->getDestPort(),
                ndpseg->getSrcPort());
    } else {
        EV_DETAIL << "ACK bit set: sending RST\n";
        sendRst(ndpseg->getAckNo(), destAddr, srcAddr, ndpseg->getDestPort(),
                ndpseg->getSrcPort());
    }
}

}// namespace ndp

} // namespace inet

