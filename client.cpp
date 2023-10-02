#include <atomic>
#include <bits/stdc++.h>
#include <queue>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BSIZE 1024
#define ulm unique_lock<mutex>


using namespace std;

typedef struct v{
    sockaddr_in socket;
    string ip;
    int cost;
} v;


char sbuffer[BSIZE];
atomic<bool> dbg = false;

queue<pair<string, int>> sq;
queue<pair<string, pair<sockaddr,socklen_t>>> rq;

mutex sq_mx;
mutex rq_mx;

condition_variable sq_cv;

int sockfd, id, port;
sockaddr_in si_local;

map<int, v> g;

map<int, v> aux;
ifstream routerFile("router.config");
ifstream linkFile("link.config");

void sender(){
    while (true) { 
        ulm lck(sq_mx);
        sq_cv.wait(lck);
        while (!sq.empty()) {
            sendto(sockfd, 
                    sq.front().first.c_str(), strlen(sq.front().first.c_str()), 
                    MSG_CONFIRM, 
                    (const struct sockaddr *) &g[sq.front().second].socket, sizeof(g[sq.front().second].socket)
                  ); 
            if(dbg) cout << "sent" << endl; 
            sq.pop();
        }
        lck.unlock();
    }
}

int receiver(){
    int n;
    sockaddr sender;
    socklen_t len;
    char rbuffer[BSIZE];
    while (true) {
        n = recvfrom(
                sockfd, 
                (char *) rbuffer, sizeof(rbuffer), 
                MSG_WAITALL, 
                (struct sockaddr *) &sender, (socklen_t *) &len
        );
        if (dbg) cout << "received" << endl;
        rbuffer[n] = '\0';
        rq_mx.lock();
        rq.push({rbuffer,{sender, len}});
        rq_mx.unlock();
    }
}


void pmenu(){
    for (auto i : g)
        cout << i.first << " " << i.second.ip << " " << ntohs(i.second.socket.sin_port) << " " << i.second.cost << endl;
    cout << endl;

    cout << "Selecionar: " << endl; 
    cout << "1) Enviar mensagem" << endl; 
    cout << "2) Mostrar mensagens" << endl;
    cout << "3) Mostrar mensagens de controle //TODO" << endl; 
    if(!dbg)
        cout << "4) Ativar debug" << endl;
    else
        cout << "4) Destivar debug" << endl;
    cout << "0) Sair" << endl;
}


// Driver code
int main() {
    cout << "Digite o ID: " << endl; 
    cin >> id;
    int a; 
    short b;
    string c;
    
    int x,y,z;
    while (linkFile >> x >> y >> z) {
        if (x == id)
            g[y].cost = z;
        if(y == id)
            g[x].cost = z;
    }

    while (routerFile >> a >> b >> c) {
        if(a == id || g.find(a)!=g.end()){
            g[a].socket.sin_family = AF_INET;
            g[a].socket.sin_port = htons(b);
            g[a].socket.sin_addr.s_addr = INADDR_ANY;
            g[a].ip = c;
        }
    }
    
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    if (bind(sockfd, (const struct sockaddr *)&g[id].socket, sizeof(g[id].socket)) < 0 ){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    thread receiverT(receiver);
    thread senderT(sender);

    int op = 1;
    while (op != 0) {
        pmenu();
        cin >> op;
        switch (op) {
            case 1:
                cout << "Enviar para: " << endl;
                cin >> a;
                if(a != -1) { 
                    cout << "Mensagem: " << endl;
                    cin >> c;
                    sq_mx.lock();
                    sq.push({c.c_str(), a});
                    sq_cv.notify_one();
                    sq_mx.unlock();
                }
                break;
            case 2:
                    rq_mx.lock();
                    while (!rq.empty()) {
                        cout << rq.front().first << endl;
                        rq.pop();
                    }
                    rq_mx.unlock();
                break;
            case 3:
                break;
            case 4:
                dbg = !dbg;
                break;
            default: break;
        } 
        cout << endl;
    }


    pthread_cancel(receiverT.native_handle());
    pthread_cancel(senderT.native_handle());
    receiverT.join();
    senderT.join();

    close(sockfd);
    return 0;
}
