//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "SimpleLoRaApp.h"
#include "inet/mobility/static/StationaryMobility.h"
#include "../LoRa/LoRaTagInfo_m.h"
#include "inet/common/packet/Packet.h"


namespace flora {

Define_Module(SimpleLoRaApp);

void SimpleLoRaApp::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    if (stage == INITSTAGE_LOCAL) {
        std::pair<double,double> coordsValues = std::make_pair(-1, -1);
        cModule *host = getContainingNode(this);
        if (strcmp(host->par("deploymentType").stringValue(), "circle")==0) {
           coordsValues = generateUniformCircleCoordinates(host->par("maxGatewayDistance").doubleValue(), host->par("gatewayX").doubleValue(), host->par("gatewayY").doubleValue());
           StationaryMobility *mobility = check_and_cast<StationaryMobility *>(host->getSubmodule("mobility"));
           mobility->par("initialX").setDoubleValue(coordsValues.first);
           mobility->par("initialY").setDoubleValue(coordsValues.second);
        }

        // Initialize new queue parameters
        maxQueueSize = par("maxQueueSize");
        currentSendingState = IDLE;
        outageInterval = par("outageInterval");
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER) {
        bool isOperational;
        NodeStatus *nodeStatus = dynamic_cast<NodeStatus *>(findContainingNode(this)->getSubmodule("status"));
        isOperational = (!nodeStatus) || nodeStatus->getState() == NodeStatus::UP;
        if (!isOperational)
            throw cRuntimeError("This module doesn't support starting in node DOWN state");

        // Schedule first outage check
        scheduleAt(simTime() + exponential(outageInterval), new cMessage("sendOutage"));

        sentPackets = 0;
        receivedADRCommands = 0;
        numberOfPacketsToSend = par("numberOfPacketsToSend");

        LoRa_AppPacketSent = registerSignal("LoRa_AppPacketSent");

        // LoRa physical layer parameters
        loRaRadio = check_and_cast<LoRaRadio *>(getParentModule()->getSubmodule("LoRaNic")->getSubmodule("radio"));
        loRaRadio->loRaTP = par("initialLoRaTP").doubleValue();
        loRaRadio->loRaCF = units::values::Hz(par("initialLoRaCF").doubleValue());
        loRaRadio->loRaSF = par("initialLoRaSF");
        loRaRadio->loRaBW = inet::units::values::Hz(par("initialLoRaBW").doubleValue());
        loRaRadio->loRaCR = par("initialLoRaCR");
        loRaRadio->loRaUseHeader = par("initialUseHeader");
        evaluateADRinNode = par("evaluateADRinNode");

        sfVector.setName("SF Vector");
        tpVector.setName("TP Vector");
    }
}

// New function to generate and queue outage packets
void SimpleLoRaApp::generateAndQueueOutagePacket()
{
    if (outageQueue.size() >= maxQueueSize) {
        EV << "Queue full, dropping outage report" << endl;
        return;
    }

    auto pkt = new Packet("OutagePacket");
    pkt->setKind(DATA);

    auto payload = makeShared<LoRaAppPacket>();
    payload->setChunkLength(B(par("dataSize").intValue()));
    payload->setSampleMeasurement(rand());
    payload->setMsgType(OUTAGE_REPORT); // this is defined in LoRaAppPacket.msg

    // Add location information
    cModule *host = getContainingNode(this);
    StationaryMobility *mobility = check_and_cast<StationaryMobility *>(host->getSubmodule("mobility"));
    Coord coord = mobility->getCurrentPosition();
    payload->setSenderX(coord.x);
    payload->setSenderY(coord.y);

    // Set LoRa parameters
    auto loraTag = pkt->addTagIfAbsent<LoRaTag>();
    loraTag->setBandwidth(getBW());
    loraTag->setCenterFrequency(getCF());
    loraTag->setSpreadFactor(getSF());
    loraTag->setCodeRendundance(getCR());
    loraTag->setPower(mW(math::dBmW2mW(getTP())));

    pkt->insertAtBack(payload);
    outageQueue.push(pkt);

    EV << "Queued outage packet, queue size now: " << outageQueue.size() << " in "<< getParentModule()->getFullName() << endl;
}

// New function to start sending from queue
void SimpleLoRaApp::startSendingFromQueue()
{
    if (currentSendingState == IDLE && !outageQueue.empty()) {
        currentSendingState = SENDING;
        scheduleAt(simTime(), new cMessage("sendPacketTimer"));
    }
}

// New function to send next packet from queue
void SimpleLoRaApp::sendNextPacketFromQueue()
{
    if (outageQueue.empty()) {
        currentSendingState = IDLE;
        return;
    }

    Packet* pkt = outageQueue.front();
    outageQueue.pop();

    send(pkt, "socketOut");
    sentPackets++;

    EV << "Sent outage packet, remaining in queue: " << outageQueue.size() << " in "<< getParentModule()->getFullName() << endl;
    emit(LoRa_AppPacketSent, getSF());

    // Schedule next packet if queue not empty
//    if (!outageQueue.empty()) {
//        double airtime = calculateLoRaAirtime();
//        scheduleAt(simTime() + airtime, new cMessage("sendPacketTimer"));
//    } else {
//        currentSendingState = IDLE;
//    }
    currentSendingState = IDLE;
}

// New function to calculate LoRa airtime
double SimpleLoRaApp::calculateLoRaAirtime()
{
    // Simplified LoRa airtime calculation
    // For more accurate calculation, use the formula from LoRa documentation
    int loRaSF = getSF();
    double bw = getBW().get() / 1000; // Convert to kHz

    // Basic airtime estimation based on SF and BW
    double symbolTime = pow(2, loRaSF) / bw;
    return symbolTime * 50; // Approximate for payload (adjust based on your packet size)
}

void SimpleLoRaApp::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        if (strcmp(msg->getName(), "sendOutage") == 0) {
            // Random outage detection (30% chance)
            if (uniform(0, 1) < 0.3) {
                EV << "[OUTAGE] Detected at node " << getParentModule()->getFullName() << endl;

                // Generate 1-3 outage reports
                int numReports = intuniform(1, 3);
                for (int i = 0; i < numReports; i++) {
                    generateAndQueueOutagePacket();
                }

                // Start sending if not already sending
                if (currentSendingState == IDLE) {
                    startSendingFromQueue();
                }
            }

            // Schedule next outage check
            scheduleAt(simTime() + exponential(outageInterval),
                      new cMessage("sendOutage"));
            delete msg;
        }
        else if (strcmp(msg->getName(), "sendPacketTimer") == 0) {
            sendNextPacketFromQueue();
            delete msg;
        }
    }
    else {
        handleMessageFromLowerLayer(msg);
        delete msg;
    }
}

// Rest of the existing functions remain the same...
std::pair<double,double> SimpleLoRaApp::generateUniformCircleCoordinates(double radius, double gatewayX, double gatewayY)
{
    double randomValueRadius = uniform(0,(radius*radius));
    double randomTheta = uniform(0,2*M_PI);

    double x = sqrt(randomValueRadius) * cos(randomTheta);
    double y = sqrt(randomValueRadius) * sin(randomTheta);
    x = x + gatewayX;
    y = gatewayY - y;
    return std::make_pair(x,y);
}

bool SimpleLoRaApp::handleOperationStage(LifecycleOperation *operation, IDoneCallback *doneCallback)
{
    Enter_Method_Silent();

    throw cRuntimeError("Unsupported lifecycle operation '%s'", operation->getClassName());
    return true;
}

void SimpleLoRaApp::finish()
{
    cModule *host = getContainingNode(this);
    StationaryMobility *mobility = check_and_cast<StationaryMobility *>(host->getSubmodule("mobility"));
    Coord coord = mobility->getCurrentPosition();
    recordScalar("positionX", coord.x);
    recordScalar("positionY", coord.y);
    recordScalar("finalTP", getTP());
    recordScalar("finalSF", getSF());
    recordScalar("sentPackets", sentPackets);
    recordScalar("receivedADRCommands", receivedADRCommands);
    recordScalar("maxQueueSizeUsed", outageQueue.size()); // Track maximum queue usage
}

void SimpleLoRaApp::handleMessageFromLowerLayer(cMessage *msg)
{
    auto pkt = check_and_cast<Packet *>(msg);
    const auto & packet = pkt->peekAtFront<LoRaAppPacket>();
    if (simTime() >= getSimulation()->getWarmupPeriod())
        receivedADRCommands++;
    if(evaluateADRinNode)
    {
        ADR_ACK_CNT = 0;
        if(packet->getMsgType() == TXCONFIG)
        {
            if(packet->getOptions().getLoRaTP() != -1)
            {
                setTP(packet->getOptions().getLoRaTP());
            }
            if(packet->getOptions().getLoRaSF() != -1)
            {
                setSF(packet->getOptions().getLoRaSF());
            }
            EV << "New TP " << getTP() << endl;
            EV << "New SF " << getSF() << endl;
        }
    }
}

void SimpleLoRaApp::increaseSFIfPossible()
{
//    if(loRaSF < 12) loRaSF++;
    if(getSF() < 12) setSF(getSF() + 1);
}

void SimpleLoRaApp::setSF(int SF) {
    loRaRadio->loRaSF = SF;
}

int SimpleLoRaApp::getSF() {
    return loRaRadio->loRaSF;
}

void SimpleLoRaApp::setTP(int TP) {
    loRaRadio->loRaTP = TP;
}

double SimpleLoRaApp::getTP() {
    return loRaRadio->loRaTP;
}

void SimpleLoRaApp::setCF(units::values::Hz CF) {
    loRaRadio->loRaCF = CF;
}

units::values::Hz SimpleLoRaApp::getCF() {
    return loRaRadio->loRaCF;
}

void SimpleLoRaApp::setBW(units::values::Hz BW) {
    loRaRadio->loRaBW = BW;
}

units::values::Hz SimpleLoRaApp::getBW() {
    return loRaRadio->loRaBW;
}

void SimpleLoRaApp::setCR(int CR) {
    loRaRadio->loRaCR = CR;
}

int SimpleLoRaApp::getCR() {
    return loRaRadio->loRaCR;
}

} //end namespace inet
