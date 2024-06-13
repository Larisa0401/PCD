/*
    Rulare
    g++ serverf.c -o serverf `pkg-config --cflags --libs opencv4`
    ./serverf 

*/
#include <stdio.h> // intrare/ieșire standard
#include <stdlib.h> // alocare și eliberare a memoriei
#include <string.h> // manipularea șirurilor de caractere
#include <unistd.h> // apeluri de sistem UNIX
#include <arpa/inet.h> // manipularea adresei IP și a porturilor
#include <pthread.h> // crearea de thread-uri
#include <opencv2/opencv.hpp> // OpenCV pentru procesarea imaginilor

#define SERVER_PORT 12349 // Definirea portului serverului
#define MAX_BUFFER_SIZE 1024 // Definirea dimensiunii maxime a bufferului

void trimite_imaginea(int client_socket, cv::Mat& imagine) { // Definirea funcției pentru trimiterea imaginii
    if (imagine.empty()) { // Verificarea dacă imaginea este goală
        fprintf(stderr, "Eroare: imaginea este goală și nu poate fi trimisă\n"); // Afișarea unei erori în cazul unei imagini goale
        return; 
    }
    std::vector<uchar> buffer; // Crearea unui vector de octeți pentru stocarea datelor imaginii
    cv::imencode(".jpg", imagine, buffer); // Codificarea imaginii în format JPEG și stocarea în buffer
    int dimensiune_imagine = buffer.size(); // Calcularea dimensiunii imaginii

    // Trimiterea dimensiunii imaginii
    if (send(client_socket, &dimensiune_imagine, sizeof(int), 0) == -1) { // Trimiterea dimensiunii imaginii prin socket
        perror("Eroare la trimiterea dimensiunii imaginii"); // Afișarea unei erori în cazul unei erori la trimitere
        return; 
    }

    // Trimiterea imaginii
    if (send(client_socket, buffer.data(), dimensiune_imagine, 0) == -1) { // Trimiterea imaginii prin socket
        perror("Eroare la trimiterea imaginii"); // Afișarea unei erori în cazul unei erori la trimitere
        return; 
    }

    printf("Imagine trimisă (%d bytes)\n", dimensiune_imagine); // Afișarea unui mesaj de confirmare a trimiterii imaginii
}

void ajusteaza_gamma(cv::Mat& imagine, double gamma, int client_socket) { // Definirea funcției pentru ajustarea gama
    if (imagine.empty()) return; // Verificarea dacă imaginea este goală
    // Construirea tabelului de tranziție gama
    unsigned char lut[256];
    for (int i = 0; i < 256; ++i) {
        lut[i] = cv::saturate_cast<unsigned char>(pow((i / 255.0), gamma) * 255.0);
    }

    // Aplicarea ajustării gama folosind tabelul de tranziție
    cv::Mat imagine_ajustata = imagine.clone();
    cv::LUT(imagine, cv::Mat(1, 256, CV_8U, lut), imagine_ajustata);

    // Trimiterea imaginii ajustate către client
    trimite_imaginea(client_socket, imagine_ajustata);
}


void decupare_imagine(cv::Mat& imagine, int client_socket) { // Definirea funcției pentru decuparea imaginii
    if (imagine.empty()) return; // Verificarea dacă imaginea este goală
    // Definirea regiunii de decupare (în acest caz, primele 100 de linii și primele 100 de coloane)
    cv::Rect roi(20, 0, 100, 100);

    // Decuparea regiunii din imagine
    cv::Mat imagine_decupata = imagine(roi).clone();

    // Trimiterea imaginii decupate către client
    trimite_imaginea(client_socket, imagine_decupata);
}

void detectare_obiecte(cv::Mat& imagine, int client_socket) { // Definirea funcției pentru detectarea obiectelor
    if (imagine.empty()) return; // Verificarea dacă imaginea este goală
    // Încărcarea clasificatorului de detectare a fețelor preantrenat
    cv::CascadeClassifier face_cascade;
    if (!face_cascade.load("haarcascade_frontalface_default.xml")) { // Încărcarea clasificatorului Haar pentru detectarea fețelor
        printf("Eroare la încărcarea clasificatorului de detectare a fețelor.\n"); // Afișarea unei erori în cazul unei probleme la încărcare
        return; // Încheierea funcției
    }

    // Convertirea imaginii în scală de gri pentru a îmbunătăți performanța
    cv::Mat imagine_gri;
    cv::cvtColor(imagine, imagine_gri, cv::COLOR_BGR2GRAY);

    // Detectarea fețelor în imagine
    std::vector<cv::Rect> fete;
    face_cascade.detectMultiScale(imagine_gri, fete, 1.1, 2, 0 | cv::CASCADE_SCALE_IMAGE, cv::Size(30, 30));

    // Desenarea dreptunghiurilor în jurul fețelor detectate
    for (size_t i = 0; i < fete.size(); ++i) {
        cv::rectangle(imagine, fete[i], cv::Scalar(255, 0, 0), 2); // Desenarea unui dreptunghi în jurul fiecărei fețe detectate
    }

    // Trimiterea imaginii cu fețele detectate către client
    trimite_imaginea(client_socket, imagine);
}

void detectare_forme(cv::Mat& imagine, int client_socket) { // Definirea funcției pentru detectarea formelor
    if (imagine.empty()) return; // Verificarea dacă imaginea este goală
    cv::Mat gri;
    cv::cvtColor(imagine, gri, cv::COLOR_BGR2GRAY);

    // Detectarea contururilor
    std::vector<std::vector<cv::Point>> contururi;
    std::vector<cv::Vec4i> ierarhie;
    cv::findContours(gri, contururi, ierarhie, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // Pentru fiecare contur detectat
    for (size_t i = 0; i < contururi.size(); ++i) {
        // Aproximarea conturului cu un poligon
        std::vector<cv::Point> aprox;
        cv::approxPolyDP(contururi[i], aprox, cv::arcLength(contururi[i], true) * 0.02, true);

        // Determinarea formei geometrice
        double aria = cv::contourArea(aprox);
        if (aprox.size() == 3) {
            // Dacă conturul are 3 laturi, este un triunghi
            cv::drawContours(imagine, contururi, i, cv::Scalar(0, 255, 0), 2); // Desenarea conturului ca triunghi
        } else if (aprox.size() == 4) {
            // Dacă conturul are 4 laturi, este un dreptunghi sau un pătrat
            double raport_aspect = std::fabs(cv::contourArea(aprox)) / (aprox.size() * aprox.size());
            if (raport_aspect >= 0.95 && raport_aspect <= 1.05) {
                // Dacă raportul aspectului este apropiat de 1, este un pătrat
                cv::drawContours(imagine, contururi, i, cv::Scalar(0, 0, 255), 2); // Desenarea conturului ca pătrat
            } else {
                // Altfel, este un dreptunghi
                cv::drawContours(imagine, contururi, i, cv::Scalar(255, 0, 0), 2); // Desenarea conturului ca dreptunghi
            }
        } else {
            // Dacă nu este triunghi, dreptunghi sau pătrat, se consideră altă formă (de exemplu, cerc)
            cv::drawContours(imagine, contururi, i, cv::Scalar(255, 255, 0), 2); // Desenarea conturului ca altă formă
        }
    }

    // Trimiterea imaginii cu contururile detectate către client
    trimite_imaginea(client_socket, imagine);
}

void ascutire_imagine(cv::Mat& imagine, int client_socket) { // Definirea funcției pentru ascuțirea imaginii
    if (imagine.empty()) return; // Verificarea dacă imaginea este goală
    // Aplicarea filtrului Laplacian pentru accentuare
    cv::Mat imagine_ascutita;
    cv::GaussianBlur(imagine, imagine_ascutita, cv::Size(0, 0), 3); // Aplicarea filtrului Gaussian pentru reducerea zgomotului
    cv::addWeighted(imagine, 1.5, imagine_ascutita, -0.5, 0, imagine_ascutita); // Adăugarea imaginii filtrate la imaginea originală cu ponderi
    // Trimiterea imaginii accentuate către client
    trimite_imaginea(client_socket, imagine_ascutita);
}


void ajusteaza_luminozitatea(cv::Mat& imagine, int luminozitate, int client_socket) { // Definirea funcției pentru ajustarea luminozității
    if (imagine.empty()) return; // Verificarea dacă imaginea este goală
    cv::Mat imagine_luminata = imagine + cv::Scalar(luminozitate, luminozitate, luminozitate); // Adăugarea unei constante la toate canalele de culoare
    trimite_imaginea(client_socket, imagine_luminata); // Trimiterea imaginii cu luminozitate ajustată către client
}

void ajusteaza_contrastul(cv::Mat& imagine, double contrast, int client_socket) { // Definirea funcției pentru ajustarea contrastului
    if (imagine.empty()) return; // Verificarea dacă imaginea este goală
    cv::Mat imagine_contrastata;
    imagine.convertTo(imagine_contrastata, -1, contrast, 0); // Conversia imaginii la alt tip cu ajustarea contrastului
    trimite_imaginea(client_socket, imagine_contrastata); // Trimiterea imaginii cu contrast ajustat către client
}

void rotire_imagine(cv::Mat& imagine, double unghi, int client_socket) { // Definirea funcției pentru rotirea imaginii
    if (imagine.empty()) return; // Verificarea dacă imaginea este goală
    cv::Point2f centru(imagine.cols / 2.0, imagine.rows / 2.0); // Calcularea centrului imaginii
    cv::Mat matrice_rotatie = cv::getRotationMatrix2D(centru, unghi, 1.0); // Obținerea matricei de rotație
    cv::Mat imagine_rotita;
    cv::warpAffine(imagine, imagine_rotita, matrice_rotatie, imagine.size()); // Aplicarea rotației pe imagine
    trimite_imaginea(client_socket, imagine_rotita); // Trimiterea imaginii rotite către client
}

void proceseaza_imaginea(const char *comanda, cv::Mat& imagine, int client_socket) { // Definirea funcției pentru procesarea imaginii
    if (strncmp(comanda, "path:", 5) == 0) { // Verificarea comenzii pentru citirea unei imagini din fișier
        const char *cale_imagine = comanda + 5;
        imagine = cv::imread(cale_imagine, cv::IMREAD_COLOR); // Citirea imaginii din fișier
        if (imagine.empty()) {
            fprintf(stderr, "Eroare la citirea imaginii: %s\n", cale_imagine); // Afișarea unei erori în caz de eșec la citirea imaginii
            return;
        }
    } else if (strcmp(comanda, "grayscale") == 0) { // Verificarea comenzii pentru convertirea în scală de gri
        if (imagine.channels() == 3) {
            cv::cvtColor(imagine, imagine, cv::COLOR_BGR2GRAY); // Convertirea imaginii în scală de gri
        }
    } else if (strcmp(comanda, "invert") == 0) { // Verificarea comenzii pentru inversarea culorilor
        cv::bitwise_not(imagine, imagine); // Aplicarea negativului imaginii
    } else if (strcmp(comanda, "detect_object") == 0) { // Verificarea comenzii pentru detectarea obiectelor
        detectare_obiecte(imagine, client_socket); // Apelarea funcției de detectare a obiectelor
    } else if (strcmp(comanda, "cropping") == 0) { // Verificarea comenzii pentru decuparea imaginii
        decupare_imagine(imagine, client_socket); // Apelarea funcției de decupare a imaginii
    } else if (strcmp(comanda, "gamma") == 0) { // Verificarea comenzii pentru ajustarea gama
        ajusteaza_gamma(imagine, 1.5, client_socket); // Apelarea funcției de ajustare a gama
    } else if (strcmp(comanda, "sharpening") == 0) { // Verificarea comenzii pentru ascuțirea imaginii
        ascutire_imagine(imagine, client_socket); // Apelarea funcției de ascuțire a imaginii
    } else if (strcmp(comanda, "see") == 0) { // Verificarea comenzii pentru afișarea imaginii
        trimite_imaginea(client_socket, imagine); // Trimiterea imaginii către client
    } else if (strcmp(comanda, "brightness") == 0) { // Verificarea comenzii pentru ajustarea luminozității
        ajusteaza_luminozitatea(imagine, 50, client_socket); // Apelarea funcției de ajustare a luminozității
    } else if (strcmp(comanda, "contrast") == 0) { // Verificarea comenzii pentru ajustarea contrastului
        ajusteaza_contrastul(imagine, 1.5, client_socket); // Apelarea funcției de ajustare a contrastului
    } else if (strcmp(comanda, "rotate") == 0) { // Verificarea comenzii pentru rotirea imaginii
        rotire_imagine(imagine, 45, client_socket); // Apelarea funcției de rotire a imaginii
    } else {
        printf("Comanda necunoscută: %s\n", comanda); // Afișarea unui mesaj în caz de comandă necunoscută
    }
}


// Funcții pentru gestionarea utilizatorilor

typedef struct { // Definirea structurii pentru utilizator
    char utilizator[256];
    char parola[256];
    char ban_status[4];
} Utilizator;

Utilizator* citeste_utilizatori(int *numar_utilizatori) { // Funcție pentru citirea utilizatorilor din fișier
    FILE *file = fopen("users.txt", "r"); // Deschiderea fișierului de utilizatori în modul citire
    if (file == NULL) { // Verificarea dacă fișierul s-a deschis cu succes
        perror("Eroare la deschiderea fișierului de utilizatori"); // Afișarea unei erori în caz de eșec la deschiderea fișierului
        return NULL;
    }

    Utilizator *utilizatori = (Utilizator*)malloc(sizeof(Utilizator) * MAX_BUFFER_SIZE); // Alocarea dinamică a memoriei pentru utilizatori
    *numar_utilizatori = 0; // Inițializarea numărului de utilizatori cu 0

    while (fscanf(file, "%255s %255s %3s", utilizatori[*numar_utilizatori].utilizator, utilizatori[*numar_utilizatori].parola, utilizatori[*numar_utilizatori].ban_status) != EOF) {
        (*numar_utilizatori)++; // Incrementarea numărului de utilizatori
        if (*numar_utilizatori >= MAX_BUFFER_SIZE) { // Verificarea dacă s-a atins limita maximă de utilizatori
            break;
        }
    }

    fclose(file); // Închiderea fișierului
    return utilizatori; // Returnarea pointerului către utilizatori
}

void actualizeaza_fisier_utilizatori(Utilizator *utilizatori, int numar_utilizatori) { // Funcție pentru actualizarea fișierului de utilizatori
    FILE *file = fopen("users.txt", "w"); // Deschiderea fișierului de utilizatori în modul scriere
    if (file == NULL) { // Verificarea dacă fișierul s-a deschis cu succes
        perror("Eroare la deschiderea fișierului de utilizatori"); // Afișarea unei erori în caz de eșec la deschiderea fișierului
        return;
    }

    for (int i = 0; i < numar_utilizatori; i++) { // Parcurgerea listei de utilizatori
        fprintf(file, "%s %s %s\n", utilizatori[i].utilizator, utilizatori[i].parola, utilizatori[i].ban_status); // Scrierea informațiilor despre utilizator în fișier
    }

    fclose(file); // Închiderea fișierului
}

void proceseaza_comanda_admin(const char *comanda, int client_socket) { // Funcție pentru procesarea comenzilor de administrare
    if (strcmp(comanda, "get_users") == 0) { // Verificarea comenzii pentru obținerea utilizatorilor
        int numar_utilizatori;
        Utilizator *utilizatori = citeste_utilizatori(&numar_utilizatori); // Citirea utilizatorilor din fișier

        // Trimiterea utilizatorilor la clientul de administrare
        for (int i = 0; i < numar_utilizatori; i++) { // Parcurgerea listei de utilizatori
            char buffer[MAX_BUFFER_SIZE];
            snprintf(buffer, sizeof(buffer), "%s %s %s", utilizatori[i].utilizator, utilizatori[i].parola, utilizatori[i].ban_status); // Construirea mesajului cu informațiile despre utilizator
            send(client_socket, buffer, strlen(buffer) + 1, 0); // Trimiterea mesajului către client
        }
        free(utilizatori); // Eliberarea memoriei alocate pentru utilizatori

        // Trimitem un mesaj pentru a indica sfârșitul listei
        send(client_socket, "end_of_list", strlen("end_of_list") + 1, 0); // Trimiterea unui mesaj special pentru a indica sfârșitul listei

    } else if (strncmp(comanda, "ban:", 4) == 0) { // Verificarea comenzii pentru banarea unui utilizator
        // Comanda pentru banarea unui utilizator
        const char *utilizator_de_banat = comanda + 4;

        int numar_utilizatori;
        Utilizator *utilizatori = citeste_utilizatori(&numar_utilizatori); // Citirea utilizatorilor din fișier

        for (int i = 0; i < numar_utilizatori; i++) { // Parcurgerea listei de utilizatori
            if (strcmp(utilizatori[i].utilizator, utilizator_de_banat) == 0) { // Verificarea dacă numele de utilizator corespunde celui specificat în comandă
                strcpy(utilizatori[i].ban_status, "ban"); // Actualizarea statusului utilizatorului la "ban"
                break;
            }
        }

        actualizeaza_fisier_utilizatori(utilizatori, numar_utilizatori); // Actualizarea fișierului de utilizatori
        free(utilizatori); // Eliberarea memoriei alocate pentru utilizatori

        send(client_socket, "user_banned", strlen("user_banned") + 1, 0); // Trimiterea unui mesaj de confirmare că utilizatorul a fost interzis
    } else {
        send(client_socket, "unknown_command", strlen("unknown_command") + 1, 0); // Trimiterea unui mesaj în caz de comandă necunoscută
    }
}


void* gestioneaza_clientul(void* arg) { // Funcție pentru gestionarea unui client
    int client_socket = *(int*)arg; // Extrageți socketul clientului din argument
    free(arg); // Eliberați memoria pentru argument

    char buffer[MAX_BUFFER_SIZE]; // Declarați un buffer pentru primirea comenzilor de la client
    cv::Mat imagine_initiala; // Declarați o matrice pentru stocarea imaginii inițiale

    // Procesarea comenzilor și trimiterea imaginilor procesate
    while (1) {
        // Primirea comenzii de la client
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0); // Primiți o comandă de la client
        if (bytes_received <= 0) { // Verificați dacă s-a primit o cantitate validă de date
            perror("Eroare la primirea comenzii"); // Afișați o eroare în caz de eșec la primirea comenzii
            close(client_socket); // Închideți socketul clientului
            return NULL; // Ieșiți din funcție
        }
        buffer[bytes_received] = '\0'; // Asigurați-vă că bufferul este terminat cu null

        // Verificați dacă comanda este pentru administrare
        if (strncmp(buffer, "admin:", 6) == 0) { // Verificați dacă comanda începe cu "admin:"
            proceseaza_comanda_admin(buffer + 6, client_socket); // Procesați comanda de administrare
        } else {
            // Procesarea imaginii conform comenzii primite
            proceseaza_imaginea(buffer, imagine_initiala, client_socket); // Procesați imaginea conform comenzii primite

            // Trimiterea imaginii procesate către client
            trimite_imaginea(client_socket, imagine_initiala); // Trimiteți imaginea procesată înapoi la client
        }
    }

    // Nu se ajunge aici, dar pentru consistență
    close(client_socket); // Închideți socketul clientului
    return NULL; // Ieșiți din funcție
}

int main() { // Funcția principală a programului
    int server_socket; // Declarați un socket pentru server
    struct sockaddr_in server_address; // Declarați o structură pentru adresa serverului

    // Crearea unui socket TCP (IPv4)
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) { // Creați un socket TCP
        perror("Eroare la crearea socketului"); // Afișați o eroare în caz de eșec la crearea socketului
        exit(EXIT_FAILURE); // Ieșiți din program cu cod de eroare
    }

    // Inițializarea structurii pentru adresa serverului
    memset(&server_address, 0, sizeof(server_address)); // Inițializați structura cu zero
    server_address.sin_family = AF_INET; // Specificați familia de adrese ca IPv4
    server_address.sin_addr.s_addr = INADDR_ANY; // Acceptați conexiuni de la orice adresă IP a serverului
    server_address.sin_port = htons(SERVER_PORT); // Specificați portul serverului

    // Legarea socketului la adresa serverului
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1) { // Legați socketul la adresa serverului
        perror("Eroare la legarea adresei"); // Afișați o eroare în caz de eșec la legarea adresei
        close(server_socket); // Închideți socketul serverului
        exit(EXIT_FAILURE); // Ieșiți din program cu cod de eroare
    }

    // Ascultarea conexiunilor
    if (listen(server_socket, 5) == -1) { // Ascultați conexiunile pe socketul serverului
        perror("Eroare la ascultarea conexiunilor"); // Afișați o eroare în caz de eșec la ascultarea conexiunilor
        close(server_socket); // Închideți socketul serverului
        exit(EXIT_FAILURE); // Ieșiți din program cu cod de eroare
    }

    printf("Serverul ascultă pe portul %d\n", SERVER_PORT); // Afișați un mesaj că serverul este pregătit să primească conexiuni

    // Acceptarea conexiunilor și gestionarea clienților
    while (1) { // Buclează în mod infinit pentru a accepta conexiuni repetate
        struct sockaddr_in client_address; // Declarați o structură pentru adresa clientului
        socklen_t client_address_len = sizeof(client_address); // Declarați o variabilă pentru lungimea adresei clientului
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_len); // Acceptați o conexiune de la un client
        if (client_socket == -1) { // Verificați dacă acceptarea conexiunii a avut succes
            perror("Eroare la acceptarea conexiunii"); // Afișați o eroare în caz de eșec la acceptarea conexiunii
            continue; // Continuați să acceptați alte conexiuni
        }

        printf("Client conectat: %s\n", inet_ntoa(client_address.sin_addr)); // Afișați adresa IP a clientului conectat

        // Creează un nou thread pentru clientul conectat
        pthread_t client_thread; // Declarați un thread pentru client
        int *client_sock_ptr = (int*)malloc(sizeof(int)); // Alocă memorie pentru pointerul la socketul clientului
        *client_sock_ptr = client_socket; // Inițializați pointerul cu socketul clientului
        if (pthread_create(&client_thread, NULL, gestioneaza_clientul, client_sock_ptr) != 0) { // Creează un thread nou pentru gestionarea clientului
            perror("Eroare la crearea thread-ului pentru client"); // Afișați o eroare în caz de eșec la crearea thread-ului
            close(client_socket); // Închideți socketul clientului
            free(client_sock_ptr); // Eliberați memoria pentru pointerul la socketul clientului
        }
        pthread_detach(client_thread); // Detachează thread-ul pentru a elibera resursele la finalizare
    }

    close(server_socket); // Închideți socketul serverului
    return 0; // Ieșiți din program cu succes
}
