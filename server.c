#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 100
#define CORRESPONDANCE_NON_TROUVEE "1"

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Arguments invalides");
        exit(EXIT_FAILURE);
    }

    // Création du socket serveur
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        fprintf(stderr,"Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse du serveur
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    // Liaison du socket à l'adresse
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        fprintf(stderr,"Erreur lors de la liaison du socket à une adresse");
        exit(EXIT_FAILURE);
    }

    // Attente de connexions
    if (listen(server_socket, 1) == -1) {
        fprintf(stderr,"Erreur lors de l'écoute");
        exit(EXIT_FAILURE);
    }

    // Acceptation d'une connexion
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_socket == -1) {
        fprintf(stderr,"Erreur lors de l'acceptation de la connexion");
        exit(EXIT_FAILURE);
    }
    printf("Connexion acceptée.\n");

    // Ouverture du fichier hash_list
    FILE *hash_file = fopen("hash_list", "r");
    if (hash_file == NULL) {
        fprintf(stderr,"Erreur lors de l'ouverture du fichier hash_list");
        exit(EXIT_FAILURE);
    }

    // Envoi des hashes du fichier hash_list au client
    char buffer[MAX_BUFFER_SIZE];
    while (fgets(buffer, sizeof(buffer), hash_file) != NULL) {
        // Envoi du hash au client
        send(client_socket, buffer, strlen(buffer), 0);
        printf("Hash envoyé au client : %s", buffer);

        // Attente de la réponse du client
        recv(client_socket, buffer, sizeof(buffer), 0);

        // Vérification si correspondance trouvée
        if (buffer[0] != CORRESPONDANCE_NON_TROUVEE[0]) {
            printf("Correspondance trouvée : %s\n", buffer);

        } else {
            printf("Aucune correspondance trouvée pour le hash : %s\n", buffer);
        }
    }
    
    // Fermeture du fichier et des socketszetox
    fclose(hash_file);
    close(client_socket);
    close(server_socket);

    return 0;
}