
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "utils.hpp"
#include "tty.hpp"

void do_tty_loop(int sockfd) {
    // Grab server socket address
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    socklen_t addr_len_orig = addr_len;
    int res = getsockname(sockfd, (sockaddr*)&addr, &addr_len);
    check("Could not get server socket address", res != 0 || addr_len != addr_len_orig);

    // Connect to server
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    check("Couldn't create socket", clientfd == -1);

    res = connect(clientfd, (sockaddr*)&addr, addr_len);
    check("Could not connect tty interface to server", res != 0);

    // Start the event loop
    char buf[512];
    char *line = NULL;
    while(1) {
        printf("> ");
        fflush(stdout);
        line = fgets(buf, sizeof(buf), stdin);
        if(line == NULL)
            break;
        
        // Process the line
        int len = strlen(line);
        if(len > 0) {
            res = write(clientfd, line, len);
            if(res != len && errno == EINTR) {
                break;
            }
            check("Could not send tty message to server", res != len);

            // Wait for response
            res = read(clientfd, buf, sizeof(buf));
            if(res < 0 && errno == EINTR) {
                break;
            }
            check("Could not read the response of the server", res < 0);
            
            // TODO: we should make sure the message was completed,
            // and read again if we only got a partial response.
        
            printf("%s\n", buf);
        }
    }

    // Cleanup
    res = close(clientfd);
    check("Could not close tty client socket", res != 0);
}

