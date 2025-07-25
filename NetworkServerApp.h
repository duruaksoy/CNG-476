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

#ifndef __LORANETWORK_NETWORKSERVERAPP_H_
#define __LORANETWORK_NETWORKSERVERAPP_H_

#include <omnetpp.h>
#include "inet/physicallayer/wireless/common/contract/packetlevel/RadioControlInfo_m.h"
#include <vector>
#include <tuple>
#include <algorithm>
#include "inet/common/INETDefs.h"

#include "LoRaMacControlInfo_m.h"
#include "LoRaMacFrame_m.h"
#include "inet/applications/base/ApplicationBase.h"
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "../LoRaApp/LoRaAppPacket_m.h"
#include <list>
#include <queue>
#include <random>

namespace flora {

class knownNode
{
public:
    MacAddress srcAddr;
    int framesFromLastADRCommand;
    int lastSeqNoProcessed;
    int numberOfSentADRPackets;
    std::list<double> adrListSNIR;
    cOutVector *historyAllSNIR;
    cOutVector *historyAllRSSI;
    cOutVector *receivedSeqNumber;
    cOutVector *calculatedSNRmargin;
};

class knownGW
{
public:
    L3Address ipAddr;
};

class receivedPacket
{
public:
    Packet* rcvdPacket = nullptr;
    cMessage* endOfWaiting = nullptr;
    std::vector<std::tuple<L3Address, double, double>> possibleGateways; // <address, sinr, rssi>
};

// In your AckTimerMessage class definition (header file)
class AckTimerMessage : public cMessage {
    private:
        Packet *ackPacket;  // Will be owned by this message until sent
        L3Address gatewayAddress;
        MacAddress nodeAddress;
    public:
        AckTimerMessage(const char *name, Packet *packet) :
            cMessage(name), ackPacket(packet)
        {
            if (ackPacket) take(ackPacket);  // Take ownership
        }

        virtual ~AckTimerMessage() {
            if (ackPacket) {
                dropAndDelete(ackPacket);  // Proper cleanup
            }
        }

        // Remove setAckPacket() since we set it in constructor
        Packet *getAckPacket() {
            Packet *pkt = ackPacket;
            ackPacket = nullptr;
            if (pkt) drop(pkt);  // Release ownership when getting it
            return pkt;
        }

        void setGatewayAddress(const L3Address &addr) { gatewayAddress = addr; }
        const L3Address &getGatewayAddress() const { return gatewayAddress; }

        void setNodeAddress(const MacAddress &addr) { nodeAddress = addr; }
        const MacAddress &getNodeAddress() const { return nodeAddress; }
};

class NetworkServerApp : public cSimpleModule, cListener
{
  protected:
    std::vector<knownNode> knownNodes;
    std::vector<knownGW> knownGateways;
    std::vector<receivedPacket> receivedPackets;
    int localPort = -1, destPort = -1;
    std::vector<std::tuple<MacAddress, int>> recvdPackets;
    // state
    UdpSocket socket;
    cMessage *selfMsg = nullptr;
    int totalReceivedPackets;
    std::string adrMethod;
    double adrDeviceMargin;
    std::map<int, int> numReceivedPerNode;
    // queue
    std::queue<Packet*> packetQueue;
    int maxQueueSize;
    int delayCount;
    simtime_t totalDelays;

    std::default_random_engine generator;
    std::uniform_real_distribution<double> delayDistribution;

  protected:
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
    void processLoraMACPacket(Packet *pk);
    void startUDP();
    void setSocketOptions();
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    bool isPacketProcessed(const Ptr<const LoRaMacFrame> &);
    void updateKnownNodes(Packet* pkt);
    void addPktToProcessingTable(Packet* pkt);
    void processScheduledPacket(cMessage* selfMsg);
    void evaluateADR(Packet *pkt, L3Address pickedGateway, double SNIRinGW, double RSSIinGW);
    void receiveSignal(cComponent *source, simsignal_t signalID, intval_t value, cObject *details) override;
    bool evaluateADRinServer;
    void processNextPacket();

    cHistogram receivedRSSI;
  public:
    simsignal_t LoRa_ServerPacketReceived;
    int counterOfSentPacketsFromNodes = 0;
    int counterOfSentPacketsFromNodesPerSF[6];
    int counterUniqueReceivedPackets = 0;
    int counterUniqueReceivedPacketsPerSF[6];

};
} //namespace inet
#endif
