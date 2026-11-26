/*
 * Esonero Reti di Calcolatori a.a. 2026-27 - Assegnazione I: socket TCP
 * Gruppo: G12
 * Componenti: Benedetto Saponaro, Arcangelo Leone, Marco Vecchi
 * File: client.c
 *
 * Client TCP: si connette al server, invia terne di interi e stampa le risposte
 * fino alla ricezione di "QUIT", poi chiude la connessione e termina.
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

#define BUFFERSIZE 512   // dimensione dei buffer di invio/ricezione

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

int main(void) {
	// Disattiva il buffer di stdout: output subito visibile nella console di Eclipse
	setvbuf(stdout, NULL, _IONBF, 0);

#if defined _WIN32
	// Inizializzazione di Winsock (versione 2.2)
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		printf("error at WSAStartup\n");
		return -1;
	}
#endif

	// Creazione della socket TCP
	int Csocket;
	Csocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (Csocket < 0) {
		ErrorHandler("socket creation failed.\n");
		ClearWinSock();
		return -1;
	}

	// Legge da tastiera IP e porta del server
	char ip[64];
	int porta;
	printf("Inserisci l'IP del server: ");
	scanf("%63s", ip);   // %63s evita di sforare l'array
	printf("Inserisci la porta del server: ");
	scanf("%d", &porta);

	// Indirizzo del server a cui connettersi
	struct sockaddr_in sad;
	memset(&sad, 0, sizeof(sad));
	sad.sin_family = AF_INET;
	sad.sin_addr.s_addr = inet_addr(ip);
	sad.sin_port = htons(porta);

	// Richiesta di connessione al server
	if (connect(Csocket, (struct sockaddr*) &sad, sizeof(sad)) < 0) {
		ErrorHandler("Connect failed\n");
		closesocket(Csocket);
		ClearWinSock();
		return -1;
	}
	printf("Connesso con il server il cui nome IP: %s\n", ip);

	// Riceve e stampa il messaggio di benvenuto
	char buf[BUFFERSIZE];
	int bytesRcvd = recv(Csocket, buf, BUFFERSIZE - 1, 0);
	if (bytesRcvd <= 0) {
		ErrorHandler("recv() failed or connection closed prematurely\n");
		closesocket(Csocket);
		ClearWinSock();
		return -1;
	}
	buf[bytesRcvd] = '\0';   // recv non aggiunge il terminatore
	printf("%s\n", buf);

	// Si ripete finche non arriva "QUIT" o si verifica un errore
	int n1, n2, n3;
	int letti = 3;
	while (letti == 3 && bytesRcvd > 0 && strcmp(buf, "QUIT") != 0) {
		printf("Inserisci tre interi n1 n2 n3: ");
		letti = scanf("%d %d %d", &n1, &n2, &n3);   // restituisce quanti valori ha letto

		if (letti == 3) {
			// I numeri viaggiano come testo separato da spazi
			char msg[BUFFERSIZE];
			sprintf(msg, "%d %d %d", n1, n2, n3);
			int len = strlen(msg);

			if (send(Csocket, msg, len, 0) != len) {
				ErrorHandler("send() sent a different number of bytes\n");
				bytesRcvd = 0;   // forza l'uscita dal ciclo
			} else {
				// Riceve la risposta del server
				bytesRcvd = recv(Csocket, buf, BUFFERSIZE - 1, 0);
				if (bytesRcvd <= 0) {
					ErrorHandler("recv() failed or connection closed prematurely\n");
				} else {
					buf[bytesRcvd] = '\0';
					if (strcmp(buf, "QUIT") == 0) {
						printf("Ricevuto: %s\n", buf);   // il ciclo termina
					} else {
						printf("Risultato: %s\n", buf);
					}
				}
			}
		} else {
			ErrorHandler("Input non valido\n");
		}
	}

	// Chiude la connessione e termina il processo
	closesocket(Csocket);
	ClearWinSock();
	return 0;
}
