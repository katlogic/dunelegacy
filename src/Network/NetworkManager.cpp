/*
 *  This file is part of Dune Legacy.
 *
 *  Dune Legacy is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Dune Legacy is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Dune Legacy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <Network/NetworkManager.h>

#include <Network/ENetHelper.h>

#include <GameInitSettings.h>

#include <globals.h>

#include <stdio.h>
#include <stdexcept>
#include <string.h>

#include <algorithm>

NetworkManager::NetworkManager(int port, std::string metaserver)
 : host(NULL), bIsServer(false), bLANServer(false), pGameInitSettings(NULL), numPlayers(0), maxPlayers(0), connectPeer(NULL), pLANGameFinderAndAnnouncer(NULL), pMetaServerClient(NULL)
{
	if(enet_initialize() != 0) {
        throw std::runtime_error("NetworkManager: An error occurred while initializing ENet.");
    }

	ENetAddress address;
	address.host = ENET_HOST_ANY;
	address.port = port;

	host = enet_host_create(&address, 32, 2, 0, 0);
	if(host == NULL) {
		enet_deinitialize();
		throw std::runtime_error("NetworkManager: An error occurred while trying to create a server host.");
	}

	if(enet_host_compress_with_range_coder(host) < 0) {
		enet_deinitialize();
		throw std::runtime_error("NetworkManager: Cannot activate range coder.");
	}

    try {
        pLANGameFinderAndAnnouncer = new LANGameFinderAndAnnouncer();
    } catch (...) {
        enet_deinitialize();
        throw;
    }

    try {
        pMetaServerClient = new MetaServerClient(metaserver);
    } catch (...) {
        delete pLANGameFinderAndAnnouncer;
        enet_deinitialize();
        throw;
    }
}


NetworkManager::~NetworkManager() {
    delete pMetaServerClient;
	delete pLANGameFinderAndAnnouncer;
	enet_host_destroy(host);
	enet_deinitialize();
}

void NetworkManager::startServer(bool bLANServer, std::string serverName, std::string playerName, GameInitSettings* pGameInitSettings, int numPlayers, int maxPlayers) {
    if(bLANServer == true) {
        if(pLANGameFinderAndAnnouncer != NULL) {
            pLANGameFinderAndAnnouncer->startAnnounce(serverName, host->address.port, pGameInitSettings->getFilename(), numPlayers, maxPlayers);
        }
    } else {
        if(pMetaServerClient != NULL) {
            pMetaServerClient->startAnnounce(serverName, host->address.port, pGameInitSettings->getFilename(), numPlayers, maxPlayers);
        }
    }

	bIsServer = true;
	this->bLANServer = bLANServer;
	this->numPlayers = numPlayers;
	this->maxPlayers = maxPlayers;
	this->playerName = playerName;
	this->pGameInitSettings = pGameInitSettings;
}

void NetworkManager::updateServer(int numPlayers) {
    if(bLANServer == true) {
        if(pLANGameFinderAndAnnouncer != NULL) {
            pLANGameFinderAndAnnouncer->updateAnnounce(numPlayers);
        }
    } else {
        if(pMetaServerClient != NULL) {
            pMetaServerClient->updateAnnounce(numPlayers);
        }
    }

	this->numPlayers = numPlayers;
}

void NetworkManager::stopServer() {
    if(bLANServer == true) {
        if(pLANGameFinderAndAnnouncer != NULL) {
            pLANGameFinderAndAnnouncer->stopAnnounce();
        }
    } else {
        if(pMetaServerClient != NULL) {
            pMetaServerClient->stopAnnounce();
        }
    }

	bIsServer = false;
	bLANServer = false;
	pGameInitSettings = NULL;
}

void NetworkManager::connect(std::string hostname, int port, std::string playerName) {
	ENetAddress address;

	if(enet_address_set_host(&address, hostname.c_str()) < 0) {
		throw std::runtime_error("NetworkManager: Resolving hostname '" + hostname + "' failed!");
	}
	address.port = port;

	connect(address, playerName);
}

void NetworkManager::connect(ENetAddress address, std::string playerName) {
    debugNetwork("Connecting to %s:%d\n", Address2String(address).c_str(), address.port);

	connectPeer = enet_host_connect(host, &address, 2, 0);
	if(connectPeer == NULL) {
		throw std::runtime_error("NetworkManager: No available peers for initiating a connection.");
	}

	this->playerName = playerName;

	connectPeer->data = new PeerData(connectPeer, PeerData::PEER_STATE_WAITING_FOR_CONNECT);
	awaitingConnectionList.push_back(connectPeer);
}

void NetworkManager::disconnect() {
   	std::list<ENetPeer*>::iterator iter;
	for(iter = awaitingConnectionList.begin(); iter != awaitingConnectionList.end(); ++iter) {
		enet_peer_disconnect_later(*iter, NETWORKDISCONNECT_QUIT);
	}
	for(iter = peerList.begin(); iter != peerList.end(); ++iter) {
		enet_peer_disconnect_later(*iter, NETWORKDISCONNECT_QUIT);
	}
}

void NetworkManager::update()
{
	if(pLANGameFinderAndAnnouncer != NULL) {
		pLANGameFinderAndAnnouncer->update();
	}

	if(pMetaServerClient != NULL) {
		pMetaServerClient->update();
	}

	if(bIsServer) {
		// Check for timeout of one client
		if(awaitingConnectionList.empty() == false) {
		    ENetPeer* pCurrentPeer = awaitingConnectionList.front();
			PeerData* peerData = (PeerData*) pCurrentPeer->data;

			if(peerData->peerState == PeerData::PEER_STATE_READY_FOR_OTHER_PEERS_TO_CONNECT) {
			    if(numPlayers >= maxPlayers) {
                    enet_peer_disconnect_later(pCurrentPeer, NETWORKDISCONNECT_GAME_FULL);
			    } else {
                    // only one peer should be in state 'PEER_STATE_WAITING_FOR_OTHER_PEERS_TO_CONNECT'
                    peerData->peerState = PeerData::PEER_STATE_WAITING_FOR_OTHER_PEERS_TO_CONNECT;
                    peerData->timeout = SDL_GetTicks() + AWAITING_CONNECTION_TIMEOUT;
                    peerData->notYetConnectedPeers = peerList;

                    if(peerData->notYetConnectedPeers.empty()) {
                        // first client on this server
                        // => change immediately to connected

                        // get change event list first
                        ChangeEventList changeEventList = pGetChangeEventListForNewPlayerCallback(peerData->name);

                        debugNetwork("Moving '%s' from awaiting connection list to peer list\n", peerData->name.c_str());
                        peerList.push_back(pCurrentPeer);
                        peerData->peerState = PeerData::PEER_STATE_CONNECTED;
                        peerData->timeout = 0;
                        awaitingConnectionList.remove(pCurrentPeer);

                        // send peer game settings
                        ENetPacketOStream packetOStream2(ENET_PACKET_FLAG_RELIABLE);
                        packetOStream2.writeUint32(NETWORKPACKET_SENDGAMEINFO);
                        pGameInitSettings->save(packetOStream2);

                        changeEventList.save(packetOStream2);

                        sendPacketToPeer(pCurrentPeer, packetOStream2);
                    } else {
                        // instruct all connected peers to connect

                        ENetPacketOStream packetOStream(ENET_PACKET_FLAG_RELIABLE);
                        packetOStream.writeUint32(NETWORKPACKET_CONNECT);
                        packetOStream.writeUint32(SDL_SwapBE32(pCurrentPeer->address.host));
                        packetOStream.writeUint16(pCurrentPeer->address.port);
                        packetOStream.writeString(peerData->name);

                        sendPacketToAllConnectedPeers(packetOStream);
                    }
			    }
			}

			if(peerData->timeout > 0 && SDL_GetTicks() > peerData->timeout) {
                // timeout
                switch(peerData->peerState) {
                    case PeerData::PEER_STATE_WAITING_FOR_NAME: {
                        // nothing to do
                    } break;

                    case PeerData::PEER_STATE_WAITING_FOR_OTHER_PEERS_TO_CONNECT: {
                        // the client awaiting connection has timed out => send everyone a disconnect message
                        ENetPacketOStream packetStream(ENET_PACKET_FLAG_RELIABLE);
                        packetStream.writeUint32(NETWORKPACKET_DISCONNECT);
                        packetStream.writeUint32(SDL_SwapBE32(pCurrentPeer->address.host));
                        packetStream.writeUint16(pCurrentPeer->address.port);

                        sendPacketToAllConnectedPeers(packetStream);

                        enet_peer_disconnect(pCurrentPeer, NETWORKDISCONNECT_TIMEOUT);

                        awaitingConnectionList.pop_front();
                    } break;

                    case PeerData::PEER_STATE_CONNECTED:
                    default: {
                        // should never happen
                    } break;
                }
			}
		}
	}

    ENetEvent event;
    while(enet_host_service(host, &event, 0) > 0) {

        ENetPeer* peer = event.peer;

        switch(event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                if(bIsServer) {
                    // Server
                    debugNetwork("NetworkManager: %s:%u connected.\n", Address2String(peer->address).c_str(), peer->address.port);

                    PeerData* newPeerData = new PeerData(peer, PeerData::PEER_STATE_WAITING_FOR_NAME);
                    newPeerData->timeout = SDL_GetTicks() + AWAITING_CONNECTION_TIMEOUT;
                    peer->data = newPeerData;

                    debugNetwork("Adding '%s' to awaiting connection list\n", newPeerData->name.c_str());
                    awaitingConnectionList.push_back(peer);

                    // Send name
                    ENetPacketOStream packetStream(ENET_PACKET_FLAG_RELIABLE);
                    packetStream.writeUint32(NETWORKPACKET_SENDNAME);
                    packetStream.writeString(playerName);

                    sendPacketToPeer(peer, packetStream);
                } else if(connectPeer != NULL) {
                    // Client
                    PeerData* peerData = (PeerData*) peer->data;

                    if(peer == connectPeer) {
                        ENetPacketOStream packetStream(ENET_PACKET_FLAG_RELIABLE);
                        packetStream.writeUint32(NETWORKPACKET_SENDNAME);
                        packetStream.writeString(playerName);

                        sendPacketToHost(packetStream);

                        peerData->peerState = PeerData::PEER_STATE_WAITING_FOR_OTHER_PEERS_TO_CONNECT;
                        peerData->timeout = 0;
                    } else {
                        debugNetwork("NetworkManager: %s:%u connected.\n", Address2String(peer->address).c_str(), peer->address.port);

                        PeerData* pConnectPeerData = (PeerData*) connectPeer->data;

                        if(pConnectPeerData->peerState == PeerData::PEER_STATE_WAITING_FOR_OTHER_PEERS_TO_CONNECT) {
                            if(peerData == NULL) {
                                peerData = new PeerData(peer, PeerData::PEER_STATE_CONNECTED);
                                peer->data = peerData;

                                debugNetwork("Adding '%s' to awaiting connection list\n", peerData->name.c_str());
                                awaitingConnectionList.push_back(peer);
                            }
                        } else {
                            ENetPacketOStream packetStream1(ENET_PACKET_FLAG_RELIABLE);
                            packetStream1.writeUint32(NETWORKPACKET_PEER_CONNECTED);
                            packetStream1.writeUint32(SDL_SwapBE32(peer->address.host));
                            packetStream1.writeUint16(peer->address.port);

                            sendPacketToHost(packetStream1);

                            ENetPacketOStream packetStream2(ENET_PACKET_FLAG_RELIABLE);
                            packetStream2.writeUint32(NETWORKPACKET_SENDNAME);
                            packetStream2.writeString(playerName);

                            sendPacketToPeer(peer, packetStream2);
                        }
                    }
                } else {
                    enet_peer_disconnect(peer, NETWORKDISCONNECT_TIMEOUT);
                }
            } break;

            case ENET_EVENT_TYPE_RECEIVE: {
                //debugNetwork("NetworkManager: A packet of length %u was received from %s:%u on channel %u on this server.\n",
                //                (unsigned int) event.packet->dataLength, Address2String(peer->address).c_str(), peer->address.port, event.channelID);

                ENetPacketIStream packetStream(event.packet);

                handlePacket(peer, packetStream);
            } break;

            case ENET_EVENT_TYPE_DISCONNECT: {
                PeerData* peerData = (PeerData*) peer->data;

                int disconnectCause = event.data;

                debugNetwork("NetworkManager: %s:%u (%s) disconnected (%d).\n", Address2String(peer->address).c_str(), peer->address.port, (peerData != NULL) ? peerData->name.c_str() : "unknown", disconnectCause);

                if(std::find(awaitingConnectionList.begin(), awaitingConnectionList.end(), peer) != awaitingConnectionList.end()) {
                    if(peerData->peerState == PeerData::PEER_STATE_WAITING_FOR_OTHER_PEERS_TO_CONNECT) {
                        ENetPacketOStream packetStream(ENET_PACKET_FLAG_RELIABLE);
                        packetStream.writeUint32(NETWORKPACKET_DISCONNECT);
                        packetStream.writeUint32(SDL_SwapBE32(peer->address.host));
                        packetStream.writeUint16(peer->address.port);

                        sendPacketToAllConnectedPeers(packetStream);
                    }

                    debugNetwork("Removing '%s' from awaiting connection list\n", peerData->name.c_str());
                    awaitingConnectionList.remove(peer);
                }


                if(std::find(peerList.begin(), peerList.end(), peer) != peerList.end()) {
                    debugNetwork("Removing '%s' from peer list\n", peerData->name.c_str());
                    peerList.remove(peer);

                    ENetPacketOStream packetStream(ENET_PACKET_FLAG_RELIABLE);
                    packetStream.writeUint32(NETWORKPACKET_DISCONNECT);
                    packetStream.writeUint32(SDL_SwapBE32(peer->address.host));
                    packetStream.writeUint16(peer->address.port);

                    sendPacketToAllConnectedPeers(packetStream);

                    if(pOnPeerDisconnected) {
                        pOnPeerDisconnected(peerData->name, (peer == connectPeer), disconnectCause);
                    }
                } else {
                    if(peer == connectPeer) {
                        // host disconnected while establishing connection
                        if(pOnPeerDisconnected) {
                            pOnPeerDisconnected(peerData->name, true, disconnectCause);
                        }
                    }
                }

                // delete peer data
                delete peerData;
                peer->data = NULL;

                if(peer == connectPeer) {
                    connectPeer = NULL;
                }

            } break;

            default: {

            } break;
        }
    }
}

void NetworkManager::handlePacket(ENetPeer* peer, ENetPacketIStream& packetStream)
{
	try {
		Uint32 packetType = packetStream.readUint32();

		switch(packetType) {
			case NETWORKPACKET_CONNECT: {

			    if(bIsServer == false) {
                    ENetAddress address;

                    address.host = SDL_SwapBE32(packetStream.readUint32());
                    address.port = packetStream.readUint16();

                    debugNetwork("Connecting to %s:%d\n", Address2String(address).c_str(), address.port);

                    ENetPeer *newPeer = enet_host_connect(host, &address, 2, 0);
                    if(newPeer == NULL) {
                        debugNetwork("NetworkManager: No available peers for initiating a connection.");
                    } else {
                        PeerData* peerData = new PeerData(newPeer, PeerData::PEER_STATE_WAITING_FOR_OTHER_PEERS_TO_CONNECT);
                        peerData->name = packetStream.readString();

                        newPeer->data = peerData;
                        debugNetwork("Adding '%s' to awaiting connection list\n", peerData->name.c_str());
                        awaitingConnectionList.push_back(newPeer);
                    }
			    }
			} break;

			case NETWORKPACKET_DISCONNECT: {
				ENetAddress address;

				address.host = SDL_SwapBE32(packetStream.readUint32());
				address.port = packetStream.readUint16();

				std::list<ENetPeer*>::iterator iter;
				for(iter = peerList.begin(); iter != peerList.end(); ++iter) {
					ENetPeer* currentPeer = *iter;
					if((currentPeer->address.host == address.host) && (currentPeer->address.port == address.port)) {
						enet_peer_disconnect_later(currentPeer, NETWORKDISCONNECT_QUIT);
						break;
					}
				}

				for(iter = awaitingConnectionList.begin(); iter != awaitingConnectionList.end(); ++iter) {
					ENetPeer* currentPeer = *iter;
					if((currentPeer->address.host == address.host) && (currentPeer->address.port == address.port)) {
						enet_peer_disconnect_later(currentPeer, NETWORKDISCONNECT_QUIT);
						break;
					}
				}

			} break;

			case NETWORKPACKET_PEER_CONNECTED: {

				ENetAddress address;

				address.host = SDL_SwapBE32(packetStream.readUint32());
				address.port = packetStream.readUint16();

				if(isServer()) {

				    if(awaitingConnectionList.empty() == false) {
                        ENetPeer* pCurrentPeer = awaitingConnectionList.front();
                        PeerData* peerData = (PeerData*) pCurrentPeer->data;

                        if((pCurrentPeer->address.host == address.host) && (pCurrentPeer->address.port == address.port)) {

                            peerData->notYetConnectedPeers.remove(peer);

                            if(peerData->notYetConnectedPeers.empty()) {
                                // send connected to all peers (excluding the new one)
                                ENetPacketOStream packetOStream(ENET_PACKET_FLAG_RELIABLE);
                                packetOStream.writeUint32(NETWORKPACKET_PEER_CONNECTED);
                                packetOStream.writeUint32(SDL_SwapBE32(pCurrentPeer->address.host));
                                packetOStream.writeUint16(pCurrentPeer->address.port);

                                sendPacketToAllConnectedPeers(packetOStream);

                                // get change event list first
                                ChangeEventList changeEventList = pGetChangeEventListForNewPlayerCallback(peerData->name);

                                // move peer to peer list
                                debugNetwork("Moving '%s' from awaiting connection list to peer list\n", peerData->name.c_str());
                                peerList.push_back(pCurrentPeer);
                                peerData->peerState = PeerData::PEER_STATE_CONNECTED;
                                peerData->timeout = 0;
                                awaitingConnectionList.remove(pCurrentPeer);

                                // send peer game settings
                                ENetPacketOStream packetOStream2(ENET_PACKET_FLAG_RELIABLE);
                                packetOStream2.writeUint32(NETWORKPACKET_SENDGAMEINFO);
                                pGameInitSettings->save(packetOStream2);

                                changeEventList.save(packetOStream2);

                                sendPacketToPeer(pCurrentPeer, packetOStream2);
                            }
                        }
				    }
				} else {
					std::list<ENetPeer*>::iterator iter;
					for(iter = awaitingConnectionList.begin(); iter != awaitingConnectionList.end(); ++iter) {
						ENetPeer* pCurrentPeer = *iter;
						PeerData* peerData = (PeerData*) pCurrentPeer->data;

						if((pCurrentPeer->address.host == address.host) && (pCurrentPeer->address.port == address.port)) {
						    debugNetwork("Moving '%s' from awaiting connection list to peer list\n", peerData->name.c_str());
						    peerList.push_back(pCurrentPeer);
                            peerData->peerState = PeerData::PEER_STATE_CONNECTED;
                            peerData->timeout = 0;
							awaitingConnectionList.erase(iter);
							break;
						}
					}
				}

			} break;

			case NETWORKPACKET_SENDGAMEINFO: {
			    PeerData* peerData = (PeerData*) connectPeer->data;

			    peerList = awaitingConnectionList;
                peerData->peerState = PeerData::PEER_STATE_CONNECTED;
                peerData->timeout = 0;
                awaitingConnectionList.clear();

                GameInitSettings gameInitSettings(packetStream);
                ChangeEventList changeEventList(packetStream);

                if(pOnReceiveGameInfo) {
                    pOnReceiveGameInfo(gameInitSettings, changeEventList);
                }
            } break;

			case NETWORKPACKET_SENDNAME: {
                PeerData* peerData = (PeerData*) peer->data;

                std::string newName = packetStream.readString();
                bool bFoundName = false;

                //check if name already exists
                if(bIsServer) {
                    if(playerName == newName) {
                        enet_peer_disconnect_later(peer, NETWORKDISCONNECT_PLAYER_EXISTS);
                        bFoundName = true;
                    }

                    if(bFoundName == false) {
                        std::list<ENetPeer*>::iterator iter;
                        for(iter = peerList.begin(); iter != peerList.end(); ++iter) {
                            PeerData* currentPeerData = (PeerData*) (*iter)->data;
                            if(currentPeerData->name == newName) {
                                enet_peer_disconnect_later(peer, NETWORKDISCONNECT_PLAYER_EXISTS);
                                bFoundName = true;
                                break;
                            }
                        }
                    }

                    if(bFoundName == false) {
                        std::list<ENetPeer*>::iterator iter;
                        for(iter = awaitingConnectionList.begin(); iter != awaitingConnectionList.end(); ++iter) {
                            PeerData* currentPeerData = (PeerData*) (*iter)->data;
                            if(currentPeerData->name == newName) {
                                enet_peer_disconnect_later(peer, NETWORKDISCONNECT_PLAYER_EXISTS);
                                bFoundName = true;
                                break;
                            }
                        }
                    }
                }

                if(bFoundName == false) {
                    peerData->name = newName;

                    if(peerData->peerState == PeerData::PEER_STATE_WAITING_FOR_NAME) {
                        peerData->peerState = PeerData::PEER_STATE_READY_FOR_OTHER_PEERS_TO_CONNECT;
                    }
                }
			} break;

			case NETWORKPACKET_CHATMESSAGE: {
			    PeerData* peerData = (PeerData*) peer->data;

				std::string message = packetStream.readString();
				if(pOnReceiveChatMessage) {
                    pOnReceiveChatMessage(peerData->name, message);
				}
			} break;

			case NETWORKPACKET_CHANGEEVENTLIST: {
			    ChangeEventList changeEventList(packetStream);

				if(pOnReceiveChangeEventList) {
                    pOnReceiveChangeEventList(changeEventList);
				}
			} break;

			case NETWORKPACKET_STARTGAME: {
			    Uint32 timeLeft = packetStream.readUint32();

                if(pOnStartGame) {
                    pOnStartGame(timeLeft);
                }
			} break;

			case NETWORKPACKET_COMMANDLIST: {
			    PeerData* peerData = (PeerData*) peer->data;

			    CommandList commandList(packetStream);

				if(pOnReceiveCommandList) {
                    pOnReceiveCommandList(peerData->name, commandList);
				}
			} break;

			case NETWORKPACKET_SELECTIONLIST: {
			    PeerData* peerData = (PeerData*) peer->data;

                int groupListIndex = packetStream.readSint32();
			    std::set<Uint32> selectedList = packetStream.readUint32Set();

				if(pOnReceiveSelectionList) {
                    pOnReceiveSelectionList(peerData->name, selectedList, groupListIndex);
				}
			} break;

			default: {
				fprintf(stderr,"NetworkManager: Unknown packet type %d\n", packetType);
			};
		}

	} catch (InputStream::eof&) {
		fprintf(stderr,"NetworkManager: Received packet is too small\n");
		return;
	} catch (std::exception& e) {
		fprintf(stderr,"NetworkManager: %s\n", e.what());
	}
}


void NetworkManager::sendPacketToHost(ENetPacketOStream& packetStream, int channel) {
	if(connectPeer == NULL) {
		fprintf(stderr,"NetworkManager: sendPacketToHost() called on server!\n");
		return;
	}

	ENetPacket* enetPacket = packetStream.getPacket();

	if(enet_peer_send(connectPeer, channel, enetPacket) < 0) {
		fprintf(stderr,"NetworkManager: Cannot send packet!\n");
	}
}

void NetworkManager::sendPacketToPeer(ENetPeer* peer, ENetPacketOStream& packetStream, int channel) {
	ENetPacket* enetPacket = packetStream.getPacket();

	if(enet_peer_send(peer, channel, enetPacket) < 0) {
		fprintf(stderr,"NetworkManager: Cannot send packet!\n");
	}

    if(enetPacket->referenceCount == 0) {
		enet_packet_destroy(enetPacket);
	}
}


void NetworkManager::sendPacketToAllConnectedPeers(ENetPacketOStream& packetStream, int channel) {
	ENetPacket* enetPacket = packetStream.getPacket();

	std::list<ENetPeer*>::iterator iter;
	for(iter = peerList.begin(); iter != peerList.end(); ++iter) {
		if(enet_peer_send(*iter, channel, enetPacket) < 0) {
			fprintf(stderr,"NetworkManager: Cannot send packet!\n");
			continue;
		}
	}

    if(enetPacket->referenceCount == 0) {
		enet_packet_destroy(enetPacket);
	}
}


void NetworkManager::sendChatMessage(const std::string& message)
{
	ENetPacketOStream packetStream(ENET_PACKET_FLAG_RELIABLE);
	packetStream.writeUint32(NETWORKPACKET_CHATMESSAGE);
	packetStream.writeString(message);

	sendPacketToAllConnectedPeers(packetStream);
}

void NetworkManager::sendChangeEventList(const ChangeEventList& changeEventList)
{
	ENetPacketOStream packetStream(ENET_PACKET_FLAG_RELIABLE);
	packetStream.writeUint32(NETWORKPACKET_CHANGEEVENTLIST);
	changeEventList.save(packetStream);

    if(bIsServer) {
        sendPacketToAllConnectedPeers(packetStream);
    } else {
        sendPacketToHost(packetStream);
    }
}

void NetworkManager::sendStartGame(unsigned int timeLeft) {
	std::list<ENetPeer*>::iterator iter;
	for(iter = peerList.begin(); iter != peerList.end(); ++iter) {
        ENetPacketOStream packetStream(ENET_PACKET_FLAG_RELIABLE);
        packetStream.writeUint32(NETWORKPACKET_STARTGAME);

        packetStream.writeUint32(timeLeft - (*iter)->roundTripTime/2);

        sendPacketToPeer(*iter, packetStream);
	}
}

void NetworkManager::sendCommandList(const CommandList& commandList) {
	ENetPacketOStream packetStream(ENET_PACKET_FLAG_UNSEQUENCED);
	packetStream.writeUint32(NETWORKPACKET_COMMANDLIST);
	commandList.save(packetStream);

    sendPacketToAllConnectedPeers(packetStream, 1);
}

void NetworkManager::sendSelectedList(const std::set<Uint32>& selectedList, int groupListIndex) {
	ENetPacketOStream packetStream(ENET_PACKET_FLAG_RELIABLE);
	packetStream.writeUint32(NETWORKPACKET_SELECTIONLIST);
	packetStream.writeSint32(groupListIndex);
	packetStream.writeUint32Set(selectedList);

    sendPacketToAllConnectedPeers(packetStream, 0);
}

int NetworkManager::getMaxPeerRoundTripTime() {
    int maxPeerRTT = 0;

	std::list<ENetPeer*>::iterator iter;
	for(iter = peerList.begin(); iter != peerList.end(); ++iter) {
	    maxPeerRTT = std::max(maxPeerRTT, (int) ((*iter)->roundTripTime));
	}

	return maxPeerRTT;
}

void NetworkManager::debugNetwork(const char* fmt, ...) {
    if(settings.network.debugNetwork) {
        va_list args;
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
    }
}
