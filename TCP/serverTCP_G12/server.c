/*
 * Esonero Reti di Calcolatori a.a. 2026-27 - Assegnazione I: socket TCP
 * Gruppo: G12
 * Componenti: Benedetto Saponaro, Arcangelo Leone, Marco Vecchi
 * File: server.c
 *
 * Server TCP iterativo: accetta un client alla volta, riceve terne di interi,
 * calcola n1+n2-n3 e risponde con il risultato oppure con "QUIT" se vale 100.
 */

#if defined _WIN32
	#include <winsock2.h>
#else
	#define closesocket close
	#include <sys/socket.h>
	#include <arpa/inet.h>
	#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROTOPORT 27015   // porta di default se non passata da riga di comando
#define QLEN 6            // lunghezza massima della coda di connessioni
#define BUFFERSIZE 512    // dimensione dei buffer di invio/ricezione

// Stampa un messaggio di errore
void ErrorHandler(char *errorMessage) {
	printf("%s", errorMessage);
}

// Libera Winsock (necessario solo su Windows)
void ClearWinSock() {
#if defined _WIN32
	WSACleanup();
#endif
}

int main(int argc, char *argv[]) {
	// Disattiva il buffer di stdout: output subito visibile nella console di Eclipse
	setvbuf(stdout, NULL, _IONBF, 0);

	// Porta: dal primo argomento se presente, altrimenti quella di default
	int port;
	if (argc > 1) {
		port = atoi(argv[1]);
	} else {
		port = PROTOPORT;
	}
	if (port < 0) {
		printf("bad port number %s \n", argv[1]);
		return 0;
	}

#if defined _WIN32
	// Inizializzazione di Winsock (versione 2.2)
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		ErrorHandler("Error at WSAStartup()\n");
		return 0;
	}
#endif

	// Creazione della socket TCP
	int MySocket;
	MySocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (MySocket < 0) {
		ErrorHandler("socket creation failed.\n");
		ClearWinSock();
		return -1;
	}

	// Indirizzo locale: IP di loopback e porta scelta (in formato di rete)
	struct sockaddr_in sad;
	memset(&sad, 0, sizeof(sad));
	sad.sin_family = AF_INET;
	sad.sin_addr.s_addr = inet_addr("127.0.0.1");
	sad.sin_port = htons(port);

	// Associa l'indirizzo alla socket
	if (bind(MySocket, (struct sockaddr*) &sad, sizeof(sad)) < 0) {
		ErrorHandler("Bind fallita.\n");
		closesocket(MySocket);
		ClearWinSock();
		return -1;
	}

	// Mette la socket in ascolto
	if (listen(MySocket, QLEN) < 0) {
		ErrorHandler("Listen fallita.\n");
		closesocket(MySocket);
		ClearWinSock();
		return -1;
	}

	struct sockaddr_in cad;   // indirizzo del client
	int clientSocket;         // socket dedicata al client connesso
	int clientLen;
	printf("Server in ascolto sulla porta %d...\n", port);

	// Il server non termina mai: dopo ogni client torna in attesa
	while (1) {
		clientLen = sizeof(cad);
		clientSocket = accept(MySocket, (struct sockaddr*) &cad, &clientLen);
		if (clientSocket < 0) {
			ErrorHandler("Accept failed\n");
			closesocket(MySocket);
			ClearWinSock();
			return -1;
		}

		// Stampa l'IP del client connesso
		printf("connesso con il client il cui nome IP: %s\n", inet_ntoa(cad.sin_addr));

		// Invia il messaggio di benvenuto
		char* buf = "connessione avvenuta";
		int stringLen = strlen(buf);
		if (send(clientSocket, buf, stringLen, 0) != stringLen) {
			ErrorHandler("send() sent a different number of bytes\n");
		} else {
			char bufRx[BUFFERSIZE];
			int res = 0;

			// Prima ricezione; le successive avvengono a fine di ogni giro
			int bytesRcvd = recv(clientSocket, bufRx, BUFFERSIZE - 1, 0);

			// Si esce se il client si disconnette, c'e un errore o res vale 100
			while (bytesRcvd > 0 && res != 100) {
				bufRx[bytesRcvd] = '\0';   // recv non aggiunge il terminatore

				// I tre interi arrivano come testo "n1 n2 n3"
				int n1, n2, n3;
				if (sscanf(bufRx, "%d %d %d", &n1, &n2, &n3) == 3) {
					res = n1 + n2 - n3;
					printf("la somma di %d+%d-%d=%d\n", n1, n2, n3, res);

					// Risponde "QUIT" se res vale 100, altrimenti con il risultato
					char risposta[BUFFERSIZE];
					if (res == 100) {
						strcpy(risposta, "QUIT");
					} else {
						sprintf(risposta, "%d", res);
					}

					int lenRisp = strlen(risposta);
					if (send(clientSocket, risposta, lenRisp, 0) != lenRisp) {
						ErrorHandler("send() sent a different number of bytes\n");
						bytesRcvd = 0;   // forza l'uscita dal ciclo
					} else if (res != 100) {
						// Attende la terna successiva dallo stesso client
						bytesRcvd = recv(clientSocket, bufRx, BUFFERSIZE - 1, 0);
					}
				} else {
					ErrorHandler("Dati ricevuti non validi\n");
					bytesRcvd = 0;   // forza l'uscita dal ciclo
				}
			}
		}

		// Chiude solo la socket del client; quella in ascolto resta aperta
		closesocket(clientSocket);
	}

	// Non raggiunto: il ciclo sopra e infinito
	closesocket(MySocket);
	ClearWinSock();
	return 0;
}
