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

package smartgrid;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.node.inet.Router;
import flora.LoRaPhy.LoRaMedium;
import flora.LoraNode.LoRaNode;
import flora.LoraNode.LoRaGW;
import inet.node.ethernet.Eth1G;
import inet.node.internetcloud.InternetCloud;
import inet.node.inet.StandardHost;
import inet.applications.udpapp.UdpBasicApp;

@license(LGPL);
network SmartGridNetwork
{
    parameters:
        @display("bgb=800,600");

    submodules:
        house1: LoRaNode {
            @display("p=265,411");
        }
        house2: LoRaNode {
            @display("p=363,411");
        }
        house3: LoRaNode {
            @display("p=465,411");
        }
        monitoring_system: LoRaGW {
            @display("p=363,316");
        }
        LoRaMedium: LoRaMedium {
            @display("p=544,315");
        }
        configurator: Ipv4NetworkConfigurator {
            parameters:
                assignDisjunctSubnetAddresses = false;
                @display("p=363,21");
        }
        ts_Router: Router {
            @display("p=143,44");
        }
        gwRouter: Router {
            @display("p=363,185");
        }
        internetCloud: InternetCloud {
            @display("p=231,44");
        }
        networkServer: StandardHost {
            parameters:
                @display("p=49,44");
        }
    connections:
        networkServer.ethg++ <--> Eth1G <--> ts_Router.ethg++;
        ts_Router.pppg++ <--> Eth1G <--> internetCloud.pppg++;
        internetCloud.pppg++ <--> Eth1G <--> gwRouter.pppg++;
        gwRouter.ethg++ <--> Eth1G <--> monitoring_system.ethg++;
}
