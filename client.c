#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/evp.h>

#define MAX_BUFFER_SIZE 100
#define CORRESPONDANCE_NON_TROUVEE "1"

// Hash le message reçu et compare avec contenu fichier
// Return 1 si correspondance non trouvée
// Return ligne + hash si correspondance trouvée
char* hash_compare(FILE *file, const char *buffer) {
    char *line = NULL;
    size_t line_length;
    EVP_MD_CTX *tmp_mdctx;
    tmp_mdctx = EVP_MD_CTX_new();
    unsigned char mdvalue[EVP_MAX_MD_SIZE];
    unsigned int mdlen;

    fseek(file, 0, SEEK_SET);

    while (getline(&line, &line_length, file) != -1) {
        // Supprimer le saut de ligne de la fin de la ligne
        size_t line_length = strlen(line);
        if (line_length > 0 && line[line_length - 1] == '\n') {
            line[line_length - 1] = '\0';
            line_length--;
        }

        // Calcul du condensat MD5
        EVP_DigestInit(tmp_mdctx, EVP_md5());
        EVP_DigestUpdate(tmp_mdctx, line, line_length);
        EVP_DigestFinal(tmp_mdctx, mdvalue, &mdlen);

        // Convertir mdvalue en une chaîne de caractères mdstring
        char mdstring[2 * mdlen + 1];
        for (size_t i = 0; i < mdlen; ++i) {
            sprintf(mdstring + 2 * i, "%02x", mdvalue[i]);
        }
        mdstring[2 * mdlen] = '\0'; // Ajouter le caractère nul à la fin

        // Comparaison entre buffer et hash
        if (memcmp(buffer, mdstring, 2 * mdlen) == 0) {
            // Si correspondance trouvée, fusionne la ligne et le hash avec séparateur
            char separation[] = " - ";
            char *result = malloc(line_length + strlen(separation) + strlen(mdstring) + 1);

            if (result == NULL) {
                fprintf(stderr, "Erreur lors de l'allocation de mémoire");
                exit(EXIT_FAILURE);
            }

            strcpy(result, line);
            strcat(result, separation);
            strcat(result, mdstring);

            free(line);
            rewind(file);
            return result;
        }
    }

    free(line);
    rewind(file);
    return NULL;
}



int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Arguments invalides");
        exit(EXIT_FAILURE);
    }

    // Création du socket client
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        fprintf(stderr,"Erreur lors de la création du socket");
        exit(EXIT_FAILURE);
    }

    // Configuration de l'adresse du serveur
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(atoi(argv[2]));

    // Connexion au serveur
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        fprintf(stderr,"Erreur lors de la connexion au serveur");
        exit(EXIT_FAILURE);
    }
    printf("Connexion au serveur établie.\n");

    // Ouverture du fichier passwd_list
    FILE *file = fopen("passwd_list", "r");
    if (file == NULL) {
        fprintf(stderr,"Erreur lors de l'ouverture du fichier hash_list");
        exit(EXIT_FAILURE);
    }

    // Réception et traitement des messages du serveur
    char buffer[MAX_BUFFER_SIZE];
    while (1) {
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            break;
        }

        // Hash et compare avec fonction récursive
        char *result = hash_compare(file, buffer);

        // Envoi de la réponse au serveur
        if (result != NULL) {
            send(client_socket, result, strlen(result), 0);

            printf("Message envoyé au serveur : %s\n", result);

            free(result);

        } else {
            // Envoie 1, si correspondance non trouvée
            send(client_socket, CORRESPONDANCE_NON_TROUVEE, strlen(CORRESPONDANCE_NON_TROUVEE), 0);
            printf("Aucune correspondance trouvée. Message envoyé au serveur : %s\n", CORRESPONDANCE_NON_TROUVEE);
        }
    }

    fclose(file);
    close(client_socket);

    return 0;
}