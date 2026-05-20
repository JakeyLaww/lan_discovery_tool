#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MDNS_PORT 5353
#define MDNS_MULTICAST_GROUP "224.0.0.251"

class NetworkScanner {
private:
    int sock;
    struct sockaddr_in local_addr;
    struct ip_mreq mreq;

public:
    NetworkScanner() {
        sock = -1;
    }

    ~NetworkScanner() {
        if (sock >= 0) {
            close(sock);
        }
    }

    bool initialize() {
        // 1. Create a UDP socket
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            std::cerr << "Failed to create socket." << std::endl;
            return false;
        }

        // 2. Allow multiple programs to listen to port 5353 (crucial for mDNS)
        int reuse = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(reuse)) < 0) {
            std::cerr << "Setting SO_REUSEADDR failed." << std::endl;
            return false;
        }

        // 3. Bind the socket to any available interface on port 5353
        memset((char *)&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family = AF_INET;
        local_addr.sin_port = htons(MDNS_PORT);
        local_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
            std::cerr << "Error binding socket." << std::endl;
            return false;
        }

        // 4. Join the mDNS multicast group
        mreq.imr_multiaddr.s_addr = inet_addr(MDNS_MULTICAST_GROUP);
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0) {
            std::cerr << "Failed to join multicast group." << std::endl;
            return false;
        }

        std::cout << "Successfully bound to " << MDNS_MULTICAST_GROUP << ":" << MDNS_PORT << std::endl;
        return true;
    }

    void listen_for_traffic() {
        std::cout << "Listening for mDNS packets... (Press Ctrl+C to stop)\n" << std::endl;
        
        char buffer[1024];
        struct sockaddr_in sender_addr;
        socklen_t sender_len = sizeof(sender_addr);

        while (true) {
            // Block and wait for incoming packets
            ssize_t bytes_read = recvfrom(sock, buffer, sizeof(buffer), 0, 
                                          (struct sockaddr*)&sender_addr, &sender_len);
            
            if (bytes_read > 0) {
                std::cout << "Received " << bytes_read << " bytes from " 
                          << inet_ntoa(sender_addr.sin_addr) << std::endl;
                
                // In Phase 2, we will pass 'buffer' to a DNS parsing function here!
            }
        }
    }
};

int main() {
    NetworkScanner scanner;
    
    if (scanner.initialize()) {
        scanner.listen_for_traffic();
    }

    return 0;
}
