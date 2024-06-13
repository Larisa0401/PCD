/*
    Rulare
    g++ clientf.c -o clientf `pkg-config --cflags --libs gtk+-3.0 opencv4`
    ./clientf
*/
#include <gtk/gtk.h> // GTK pentru a crea interfața grafică
#include <stdio.h> //  intrare/ieșire
#include <stdlib.h> // alocare și eliberare a memoriei
#include <string.h> //  manipularea șirurilor de caractere
#include <unistd.h> //  funcții de sistem POSIX
#include <arpa/inet.h> //  manipularea adreselor de internet
#include <sys/types.h> // Include biblioteca pentru definiții de tipuri de date
#include <sys/socket.h> //  funcții de operare pe socket
#include <pthread.h> // suportul la fire de execuție
#include <opencv2/opencv.hpp> // OpenCV pentru procesarea imaginilor

#define SERVER_IP "127.0.0.1" // Definirea adresei IP a serverului
#define SERVER_PORT 12349 // Definirea portului serverului
#define MAX_BUFFER_SIZE 1024 // Definirea dimensiunii maxime a bufferului

GtkWidget *buton_conectare; // Declarația unui buton pentru conectare
GtkWidget *buton_imagine; // Declarația unui buton pentru imagine
GtkWidget *widget_imagine_originala; // Declarația unui widget pentru imaginea originală
GtkWidget *widget_imagine_modificata; // Declarația unui widget pentru imaginea modificată
GtkWidget *buton_deconectare; // Declarația unui buton pentru deconectare
GtkWidget *buton_salvare; // Declarația unui buton pentru salvare
GtkWidget *buton_resetare; // Declarația unui buton pentru resetare
GtkWidget *fereastra_principala; // Declarația ferestrei principale
int client_socket_fd = -1; // Inițializarea variabilei pentru socketul clientului

char cale_imagine_curenta[MAX_BUFFER_SIZE]; // Declarația variabilei pentru calea imaginii curente

// Declarați butoanele lipsă ca variabile globale
GtkWidget *buton_grayscale; // Declarația butonului pentru tonuri de gri
GtkWidget *buton_invert; // Declarația butonului pentru inversare
GtkWidget *buton_detectare_obiect; // Declarația butonului pentru detectarea obiectelor
GtkWidget *buton_decupare; // Declarația butonului pentru decupare
GtkWidget *buton_gamma; // Declarația butonului pentru corecția gamma
GtkWidget *buton_ascutire; // Declarația butonului pentru ascuțire
GtkWidget *buton_vizualizare; // Declarația butonului pentru vizualizare
GtkWidget *buton_selecteaza_noua_imagine; // Declarația butonului pentru selectarea unei noi imagini
GtkWidget *buton_estompare; // Declarația butonului pentru estompare
GtkWidget *buton_sepia; // Declarația butonului pentru efectul de sepia
GtkWidget *buton_margini; // Declarația butonului pentru detecția de margini
GtkWidget *buton_luminozitate; // Declarația butonului pentru ajustarea luminozității
GtkWidget *buton_contrast; // Declarația butonului pentru ajustarea contrastului
GtkWidget *buton_rotire; // Declarația butonului pentru rotirea imaginii

// Declarați widget-urile pentru logare și înregistrare
GtkWidget *fereastra_logare; // Declarația ferestrei pentru logare
GtkWidget *fereastra_inregistrare; // Declarația ferestrei pentru înregistrare
GtkWidget *camp_utilizator; // Declarația câmpului pentru numele de utilizator
GtkWidget *camp_parola; // Declarația câmpului pentru parolă

// Funcții pentru gestionarea fișierului de autentificare
void inregistreaza_utilizator(const char *utilizator, const char *parola) { // Funcție pentru înregistrarea utilizatorului
    FILE *file = fopen("users.txt", "a"); // Deschide fișierul pentru adăugarea de conținut
    if (file == NULL) { // Verifică dacă deschiderea fișierului a eșuat
        perror("Eroare la deschiderea fișierului de utilizatori"); // Afișează eroarea
        return; // Încheie funcția
    }
    fprintf(file, "%s %s 0\n", utilizator, parola); // Adaugă utilizatorul în fișier cu status "0" (nebanat)
    fclose(file); // Închide fișierul
}

int valideaza_utilizator(const char *utilizator, const char *parola) { // Funcție pentru validarea utilizatorului
    FILE *file = fopen("users.txt", "r"); // Deschide fișierul pentru citire
    if (file == NULL) { // Verifică dacă deschiderea fișierului a eșuat
        perror("Eroare la deschiderea fișierului de utilizatori"); // Afișează eroarea
        return 0; // Returnează 0 pentru utilizator invalid
    }
    char utilizator_salvat[256]; // Declarația variabilei pentru utilizatorul salvat
    char parola_salvata[256]; // Declarația variabilei pentru parola salvată
    char status_ban[256]; // Declarația variabilei pentru statusul de ban
    while (fscanf(file, "%s %s %s", utilizator_salvat, parola_salvata, status_ban) != EOF) { // Parcurge fișierul până la sfârșit
        if (strcmp(utilizator, utilizator_salvat) == 0 && strcmp(parola, parola_salvata) == 0) { // Verifică dacă utilizatorul și parola sunt corecte
            if (strcmp(status_ban, "ban") == 0) { // Verifică dacă utilizatorul este banat
                fclose(file); // Închide fișierul
                return -1; // Returnează -1 pentru utilizator banat
            }
            fclose(file); // Închide fișierul
            return 1; // Returnează 1 pentru utilizator valid
        }
    }
    fclose(file); // Închide fișierul
    return 0; // Returnează 0 pentru utilizator sau parolă incorecte
}

// Funcțiile pentru logare și înregistrare
void pe_buton_logare_apasat(GtkWidget *widget, gpointer data) { // Funcție apelată când este apăsat butonul de logare
    const char *utilizator = gtk_entry_get_text(GTK_ENTRY(camp_utilizator)); // Obține numele de utilizator introdus
    const char *parola = gtk_entry_get_text(GTK_ENTRY(camp_parola)); // Obține parola introdusă
    int validare = valideaza_utilizator(utilizator, parola); // Validează utilizatorul și parola
    if (validare == 1) { // Dacă utilizatorul este valid
        gtk_widget_hide(fereastra_logare); // Ascunde fereastra de logare
        gtk_widget_show_all(fereastra_principala); // Afișează fereastra principală
    } else if (validare == -1) { // Dacă utilizatorul este banat
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(fereastra_logare), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Acces interzis! Ați fost banat.");
        gtk_dialog_run(GTK_DIALOG(dialog)); // Rulează dialogul
        gtk_widget_destroy(dialog); // Distruge dialogul
    } else { // Dacă utilizatorul sau parola sunt incorecte
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(fereastra_logare), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Autentificare eșuată!");
        gtk_dialog_run(GTK_DIALOG(dialog)); // Rulează dialogul
        gtk_widget_destroy(dialog); // Distruge dialogul
    }
}

void pe_buton_inregistrare_apasat(GtkWidget *widget, gpointer data) { // Funcție apelată când este apăsat butonul de înregistrare
    gtk_widget_show_all(fereastra_inregistrare); // Afișează fereastra de înregistrare
}

void pe_buton_confirmare_inregistrare_apasat(GtkWidget *widget, gpointer data) { // Funcție apelată când este apăsat butonul de confirmare a înregistrării
    GtkWidget *camp_utilizator = (GtkWidget *)g_object_get_data(G_OBJECT(widget), "camp_utilizator"); // Obține câmpul pentru numele de utilizator
    GtkWidget *camp_parola = (GtkWidget *)g_object_get_data(G_OBJECT(widget), "camp_parola"); // Obține câmpul pentru parolă

    const char *utilizator = gtk_entry_get_text(GTK_ENTRY(camp_utilizator)); // Obține numele de utilizator introdus
    const char *parola = gtk_entry_get_text(GTK_ENTRY(camp_parola)); // Obține parola introdusă

    inregistreaza_utilizator(utilizator, parola); // Înregistrează utilizatorul
    gtk_widget_hide(fereastra_inregistrare); // Ascunde fereastra de înregistrare
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(fereastra_inregistrare), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "Înregistrare reușită!");
    gtk_dialog_run(GTK_DIALOG(dialog)); // Rulează dialogul
    gtk_widget_destroy(dialog); // Distruge dialogul
}

// Funcția pentru crearea ferestrei de logare
void creeaza_fereastra_logare() { // Funcție pentru crearea ferestrei de logare
    fereastra_logare = gtk_window_new(GTK_WINDOW_TOPLEVEL); // Creează fereastra de top pentru logare
    gtk_window_set_title(GTK_WINDOW(fereastra_logare), "Logare"); // Setează titlul ferestrei
    gtk_container_set_border_width(GTK_CONTAINER(fereastra_logare), 20); // Setează lățimea marginilor ferestrei
    gtk_widget_set_size_request(fereastra_logare, 300, 200); // Setează dimensiunile ferestrei

    GtkWidget *caseta_logare = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10); // Creează o casetă verticală pentru logare
    gtk_container_add(GTK_CONTAINER(fereastra_logare), caseta_logare); // Adaugă caseta în fereastra de logare

    camp_utilizator = gtk_entry_new(); // Creează un câmp de introducere pentru numele de utilizator
    gtk_entry_set_placeholder_text(GTK_ENTRY(camp_utilizator), "Utilizator"); // Setează textul de substituție pentru numele de utilizator
    gtk_box_pack_start(GTK_BOX(caseta_logare), camp_utilizator, FALSE, FALSE, 0); // Adaugă câmpul în casetă

    camp_parola = gtk_entry_new(); // Creează un câmp de introducere pentru parolă
    gtk_entry_set_placeholder_text(GTK_ENTRY(camp_parola), "Parola"); // Setează textul de substituție pentru parolă
    gtk_entry_set_visibility(GTK_ENTRY(camp_parola), FALSE); // Face parola invizibilă
    gtk_box_pack_start(GTK_BOX(caseta_logare), camp_parola, FALSE, FALSE, 0); // Adaugă câmpul în casetă

    GtkWidget *buton_logare = gtk_button_new_with_label("Logare"); // Creează un buton cu eticheta "Logare"
    g_signal_connect(buton_logare, "clicked", G_CALLBACK(pe_buton_logare_apasat), NULL); // Conectează semnalul "clicked" la funcția de apel
    gtk_box_pack_start(GTK_BOX(caseta_logare), buton_logare, FALSE, FALSE, 0); // Adaugă butonul în casetă

    GtkWidget *buton_inregistrare = gtk_button_new_with_label("Înregistrare"); // Creează un buton cu eticheta "Înregistrare"
    g_signal_connect(buton_inregistrare, "clicked", G_CALLBACK(pe_buton_inregistrare_apasat), NULL); // Conectează semnalul "clicked" la funcția de apel
    gtk_box_pack_start(GTK_BOX(caseta_logare), buton_inregistrare, FALSE, FALSE, 0); // Adaugă butonul în casetă

    gtk_widget_show_all(fereastra_logare); // Afișează toate widget-urile din fereastra de logare
}

// Funcția pentru crearea ferestrei de înregistrare
void creeaza_fereastra_inregistrare() { // Funcție pentru crearea ferestrei de înregistrare
    fereastra_inregistrare = gtk_window_new(GTK_WINDOW_TOPLEVEL); // Creează fereastra de top pentru înregistrare
    gtk_window_set_title(GTK_WINDOW(fereastra_inregistrare), "Înregistrare"); // Setează titlul ferestrei
    gtk_container_set_border_width(GTK_CONTAINER(fereastra_inregistrare), 20); // Setează lățimea marginilor ferestrei
    gtk_widget_set_size_request(fereastra_inregistrare, 300, 200); // Setează dimensiunile ferestrei

    GtkWidget *caseta_inregistrare = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10); // Creează o casetă verticală pentru înregistrare
    gtk_container_add(GTK_CONTAINER(fereastra_inregistrare), caseta_inregistrare); // Adaugă caseta în fereastra de înregistrare

    GtkWidget *camp_utilizator = gtk_entry_new(); // Creează un câmp de introducere pentru numele de utilizator
    gtk_entry_set_placeholder_text(GTK_ENTRY(camp_utilizator), "Utilizator"); // Setează textul de substituție pentru numele de utilizator
    gtk_box_pack_start(GTK_BOX(caseta_inregistrare), camp_utilizator, FALSE, FALSE, 0); // Adaugă câmpul în casetă

    GtkWidget *camp_parola = gtk_entry_new(); // Creează un câmp de introducere pentru parolă
    gtk_entry_set_placeholder_text(GTK_ENTRY(camp_parola), "Parola"); // Setează textul de substituție pentru parolă
    gtk_entry_set_visibility(GTK_ENTRY(camp_parola), FALSE); // Face parola invizibilă
    gtk_box_pack_start(GTK_BOX(caseta_inregistrare), camp_parola, FALSE, FALSE, 0); // Adaugă câmpul în casetă

    GtkWidget *buton_confirmare_inregistrare = gtk_button_new_with_label("Înregistrează-te"); // Creează un buton cu eticheta "Înregistrează-te"
    g_object_set_data(G_OBJECT(buton_confirmare_inregistrare), "camp_utilizator", camp_utilizator); // Setează datele utilizatorului pentru buton
    g_object_set_data(G_OBJECT(buton_confirmare_inregistrare), "camp_parola", camp_parola); // Setează datele parolei pentru buton
    g_signal_connect(buton_confirmare_inregistrare, "clicked", G_CALLBACK(pe_buton_confirmare_inregistrare_apasat), NULL); // Conectează semnalul "clicked" la funcția de apel
    gtk_box_pack_start(GTK_BOX(caseta_inregistrare), buton_confirmare_inregistrare, FALSE, FALSE, 0); // Adaugă butonul în casetă
}

// Funcțiile de bază pentru manipularea imaginilor
ssize_t primeste_tot(int socket, void *buffer, size_t length) { // Funcție pentru primirea completă a datelor prin socket
    size_t total_primit = 0; // Inițializarea variabilei pentru totalul datelor primite
    ssize_t bytes_primit; // Declarația variabilei pentru numărul de octeți primiți
    while (total_primit < length) { // Parcurge până când se primește întreaga lungime specificată
        bytes_primit = recv(socket, (char*)buffer + total_primit, length - total_primit, 0); // Primește datele
        if (bytes_primit == -1) { // Verifică dacă a apărut o eroare la primire
            perror("Eroare la primirea datelor"); // Afișează eroarea
            return -1; // Încheie funcția cu eroare
        } else if (bytes_primit == 0) { // Verifică dacă conexiunea s-a închis
            break; // Încheie bucla
        }
        total_primit += bytes_primit; // Adaugă la totalul datelor primite
    }
    return total_primit; // Returnează numărul total de octeți primiți
}

void *conecteaza_la_server(void *arg) { // Funcție pentru conectarea la server
    struct sockaddr_in adresa_server; // Declarația structurii pentru adresa serverului

    // Crearea unui socket TCP (IPv4)
    if ((client_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { // Verifică și creează socketul
        perror("Eroare la crearea socketului"); // Afișează eroarea
        exit(EXIT_FAILURE); // Ieșire cu eroare
    }

    // Inițializarea structurii pentru adresa serverului
    memset(&adresa_server, 0, sizeof(adresa_server)); // Inițializează structura cu zero
    adresa_server.sin_family = AF_INET; // Setează familia de adrese la IPv4
    adresa_server.sin_addr.s_addr = inet_addr(SERVER_IP); // Setează adresa IP a serverului
    adresa_server.sin_port = htons(SERVER_PORT); // Setează portul serverului

    // Conectarea la server
    if (connect(client_socket_fd, (struct sockaddr *)&adresa_server, sizeof(adresa_server)) == -1) { // Verifică și stabilește conexiunea
        perror("Eroare la conectarea la server"); // Afișează eroarea
        close(client_socket_fd); // Închide socketul
        client_socket_fd = -1; // Resetează valoarea socketului
        return NULL; // Încheie funcția
    }

    printf("Conectat la server %s:%d\n", SERVER_IP, SERVER_PORT); // Afișează mesajul de conectare la server

    return NULL; // Încheie funcția cu succes
}


// Funcția pentru a trimite calea către server
void trimite_calea_imaginii(const char *cale) {
    char cale_prefixata[MAX_BUFFER_SIZE];
    snprintf(cale_prefixata, sizeof(cale_prefixata), "path:%s", cale); // Construiește calea prefixată cu "path:"
    printf("Calea imaginii trimise: %s\n", cale_prefixata); // Mesaj de debug pentru verificarea căii
    size_t lungime_cale = strlen(cale_prefixata);
    if (send(client_socket_fd, cale_prefixata, lungime_cale + 1, 0) == -1) { // Trimite și terminatorul null '\0'
        perror("Eroare la trimiterea căii către server");
        exit(EXIT_FAILURE);
    }
}

void pe_buton_conectare_apasat(GtkWidget *widget, gpointer data) {
    pthread_t thread;
    pthread_create(&thread, NULL, conecteaza_la_server, NULL); // Creează un fir de execuție pentru conectarea la server
    pthread_detach(thread); // Face firul de execuție independent de thread-ul principal
    gtk_widget_hide(buton_conectare); // Ascundeți butonul de conectare după ce s-a realizat conexiunea
    gtk_widget_show(buton_imagine); // Afișează butonul de alegere a imaginii după conectare
    gtk_widget_show(buton_deconectare); // Afișează butonul de deconectare
}

// Funcția callback care va fi apelată la apăsarea butonului de alegere a imaginii
void pe_buton_imagine_apasat(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog;
    GtkFileChooserAction actiune = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    dialog = gtk_file_chooser_dialog_new("Alegeți o imagine",
                                         NULL,
                                         actiune,
                                         "_Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         "_Open",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT) {
        char *nume_fisier;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        nume_fisier = gtk_file_chooser_get_filename(chooser);
        g_print("Fișier selectat: %s\n", nume_fisier);

        strcpy(cale_imagine_curenta, nume_fisier); // Actualizare cale curentă a imaginii

        gtk_image_set_from_file(GTK_IMAGE(widget_imagine_originala), nume_fisier);
        gtk_image_set_from_file(GTK_IMAGE(widget_imagine_modificata), nume_fisier); // Setăm și imaginea modificată la aceeași cale
        trimite_calea_imaginii(nume_fisier); // Trimite calea imaginii la server
        g_free(nume_fisier);

        // Afișarea butoanelor după selectarea imaginii
        gtk_widget_show(buton_grayscale);
        gtk_widget_show(buton_invert);
        gtk_widget_show(buton_detectare_obiect);
        gtk_widget_show(buton_decupare);
        gtk_widget_show(buton_gamma);
        gtk_widget_show(buton_ascutire);
        gtk_widget_show(buton_vizualizare);
        gtk_widget_show(buton_salvare);
        gtk_widget_show(buton_resetare);
        gtk_widget_show(buton_estompare);
        gtk_widget_show(buton_sepia);
        gtk_widget_show(buton_margini);
        gtk_widget_show(buton_luminozitate);
        gtk_widget_show(buton_contrast);
        gtk_widget_show(buton_rotire);

        gtk_widget_show_all(buton_selecteaza_noua_imagine);
        gtk_widget_hide(buton_imagine);
    }

    gtk_widget_destroy(dialog);
}

void trimite_comanda(const char *comanda) {
    if (client_socket_fd == -1) {
        fprintf(stderr, "Nu sunteți conectat la server.\n");
        return;
    }

    // Trimite comanda la server
    if (send(client_socket_fd, comanda, strlen(comanda) + 1, 0) == -1) { // Trimite și terminatorul null '\0'
        perror("Eroare la trimiterea comenzii către server");
        return;
    }

    // Așteaptă să primească imaginea modificată de la server
    int dimensiune_imagine;
    if (recv(client_socket_fd, &dimensiune_imagine, sizeof(dimensiune_imagine), 0) == -1) {
        perror("Eroare la primirea dimensiunii imaginii");
        return;
    }

    // Alocă memorie pentru imagine
    char *date_imagine = (char *)malloc(dimensiune_imagine);
    if (!date_imagine) {
        perror("Eroare la alocarea memoriei pentru imagine");
        return;
    }

    // Primește imaginea modificată
    if (primeste_tot(client_socket_fd, date_imagine, dimensiune_imagine) != dimensiune_imagine) {
        perror("Eroare la primirea imaginii modificate");
        free(date_imagine);
        return;
    }

    // Salvează imaginea primită într-un fișier temporar
    char cale_imagine_temp[] = "/tmp/received_image.jpg";
    FILE *file = fopen(cale_imagine_temp, "wb");
    if (!file) {
        perror("Eroare la deschiderea fișierului temporar pentru imagine");
        free(date_imagine);
        return;
    }
    fwrite(date_imagine, 1, dimensiune_imagine, file);
    fclose(file);
    free(date_imagine);

    // Afișează imaginea modificată în widget-ul de imagine modificată
    gtk_image_set_from_file(GTK_IMAGE(widget_imagine_modificata), cale_imagine_temp);
}

// Funcția pentru salvarea imaginii
void pe_buton_salvare_apasat(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog;
    GtkFileChooserAction actiune = GTK_FILE_CHOOSER_ACTION_SAVE;
    gint res;

    dialog = gtk_file_chooser_dialog_new("Salvați imaginea",
                                         NULL,
                                         actiune,
                                         "_Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         "_Save",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dialog), TRUE);

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT) {
        char *nume_fisier;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        nume_fisier = gtk_file_chooser_get_filename(chooser);
        g_print("Salvare fișier: %s\n", nume_fisier);

        // Copiază imaginea temporară la locația selectată
        char cale_imagine_temp[] = "/tmp/received_image.jpg";
        if (rename(cale_imagine_temp, nume_fisier) != 0) {
            perror("Eroare la salvarea imaginii");
        }

        g_free(nume_fisier);
    }

    gtk_widget_destroy(dialog);
}

// Funcția pentru resetarea imaginii modificate la cea originală
void pe_buton_resetare_apasat(GtkWidget *widget, gpointer data) {
    gtk_image_set_from_file(GTK_IMAGE(widget_imagine_modificata), cale_imagine_curenta);
}

// Funcțiile callback pentru butoanele de efecte
void pe_buton_grayscale_apasat(GtkWidget *widget, gpointer data) {
    trimite_comanda("grayscale");
}

void pe_buton_invert_apasat(GtkWidget *widget, gpointer data) {
    trimite_comanda("invert");
}

void pe_buton_detectare_obiect_apasat(GtkWidget *widget, gpointer data) {
    trimite_comanda("detect_object");
}

void pe_buton_decupare_apasat(GtkWidget *widget, gpointer data) {
    trimite_comanda("cropping");
}

void pe_buton_gamma_apasat(GtkWidget *widget, gpointer data) {
    trimite_comanda("gamma");
}

void pe_buton_ascutire_apasat(GtkWidget *widget, gpointer data) {
    trimite_comanda("sharpening");
}

void pe_buton_vizualizare_apasat(GtkWidget *widget, gpointer data) {
    trimite_comanda("see");
}

void pe_buton_estompare_apasat(GtkWidget *widget, gpointer data) {
    trimite_comanda("blur");
}

void pe_buton_sepia_apasat(GtkWidget *widget, gpointer data) {
    trimite_comanda("sepia");
}

void pe_buton_margini_apasat(GtkWidget *widget, gpointer data) {
    trimite_comanda("edge_detection");
}

void pe_buton_luminozitate_apasat(GtkWidget *widget, gpointer data) {
    trimite_comanda("brightness");
}

void pe_buton_contrast_apasat(GtkWidget *widget, gpointer data) {
    trimite_comanda("contrast");
}

void pe_buton_rotire_apasat(GtkWidget *widget, gpointer data) {
    trimite_comanda("rotate");
}

// Funcția callback pentru deconectare
void pe_buton_deconectare_apasat(GtkWidget *widget, gpointer data) {
    if (client_socket_fd != -1) {
        close(client_socket_fd);
        client_socket_fd = -1;
        printf("Deconectat de la server\n");
    }
    gtk_main_quit(); // Închide aplicația
}


// Funcția callback care va fi apelată la apăsarea butonului de selectare a unei noi imagini
void pe_buton_selecteaza_noua_imagine_apasat(GtkWidget *widget, gpointer data) {
    GtkWidget *dialog;
    GtkFileChooserAction actiune = GTK_FILE_CHOOSER_ACTION_OPEN;
    gint res;

    // Crearea dialogului pentru alegerea imaginii
    dialog = gtk_file_chooser_dialog_new("Alegeți o imagine",
                                         NULL,
                                         actiune,
                                         "_Cancel",
                                         GTK_RESPONSE_CANCEL,
                                         "_Open",
                                         GTK_RESPONSE_ACCEPT,
                                         NULL);

    res = gtk_dialog_run(GTK_DIALOG(dialog));
    if (res == GTK_RESPONSE_ACCEPT) {
        char *nume_fisier;
        GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
        nume_fisier = gtk_file_chooser_get_filename(chooser);
        g_print("Fișier selectat: %s\n", nume_fisier);

        // Afișarea imaginii selectate în widget-ul de imagine originală
        gtk_image_set_from_file(GTK_IMAGE(widget_imagine_originala), nume_fisier);
        gtk_image_set_from_file(GTK_IMAGE(widget_imagine_modificata), nume_fisier); // Setăm și imaginea modificată la aceeași cale

        trimite_calea_imaginii(nume_fisier);
        g_free(nume_fisier);
    }

    gtk_widget_destroy(dialog);
}

// Funcția care va crea interfața grafică
void creeaza_interfata(int argc, char *argv[]) {
    // Inițializarea GTK
    gtk_init(&argc, &argv);

    // Crearea ferestrei principale
    fereastra_principala = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(fereastra_principala), "Conectare la server");
    gtk_container_set_border_width(GTK_CONTAINER(fereastra_principala), 20);
    gtk_widget_set_size_request(fereastra_principala, 800, 600); // Dimensiunile ferestrei
    g_signal_connect(fereastra_principala, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Setarea poziției ferestrei pe mijlocul ecranului
    gtk_window_set_position(GTK_WINDOW(fereastra_principala), GTK_WIN_POS_CENTER_ALWAYS);

    // Crearea containerului principal pentru widget-uri
    GtkWidget *caseta_principala = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_add(GTK_CONTAINER(fereastra_principala), caseta_principala);

    // Crearea containerului pentru imaginile originale și modificate
    GtkWidget *caseta_imagini = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_box_pack_start(GTK_BOX(caseta_principala), caseta_imagini, TRUE, TRUE, 0);

    // Crearea widget-urilor pentru imaginea originală și modificată
    widget_imagine_originala = gtk_image_new();
    gtk_box_pack_start(GTK_BOX(caseta_imagini), widget_imagine_originala, TRUE, TRUE, 0);

    widget_imagine_modificata = gtk_image_new();
    gtk_box_pack_start(GTK_BOX(caseta_imagini), widget_imagine_modificata, TRUE, TRUE, 0);

    // Crearea containerului pentru butoane
    GtkWidget *caseta_butoane = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_box_pack_start(GTK_BOX(caseta_principala), caseta_butoane, FALSE, FALSE, 0);

    // Crearea butonului de conectare la server
    buton_conectare = gtk_button_new_with_label("Conectare la server");
    gtk_widget_set_size_request(buton_conectare, 150, 50); // Dimensiunile butonului
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_conectare, FALSE, FALSE, 0); // Adăugarea butonului la container
    g_signal_connect(buton_conectare, "clicked", G_CALLBACK(pe_buton_conectare_apasat), NULL);

    // Crearea butonului de alegere a imaginii
    buton_imagine = gtk_button_new_with_label("Alege imagine");
    gtk_widget_set_size_request(buton_imagine, 150, 50); // Dimensiunile butonului
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_imagine, FALSE, FALSE, 0); // Adăugarea butonului la container
    gtk_widget_hide(buton_imagine); // Ascundeți butonul de alegere a imaginii inițial
    g_signal_connect(buton_imagine, "clicked", G_CALLBACK(pe_buton_imagine_apasat), NULL);

    // Crearea butoanelor de editare
    buton_grayscale = gtk_button_new_with_label("Grayscale");
    buton_invert = gtk_button_new_with_label("Invert");
    buton_detectare_obiect = gtk_button_new_with_label("Detectare Obiect");
    buton_decupare = gtk_button_new_with_label("Decupare");
    buton_gamma = gtk_button_new_with_label("Gamma");
    buton_ascutire = gtk_button_new_with_label("Ascutire");
    buton_vizualizare = gtk_button_new_with_label("Vizualizare imagini modificate");
    buton_salvare = gtk_button_new_with_label("Salvează imaginea"); // Butonul de salvare
    buton_resetare = gtk_button_new_with_label("Resetează imaginea"); // Butonul de resetare
    buton_estompare = gtk_button_new_with_label("Estompare");
    buton_sepia = gtk_button_new_with_label("Sepia");
    buton_margini = gtk_button_new_with_label("Detecție Margini");
    buton_luminozitate = gtk_button_new_with_label("Luminozitate");
    buton_contrast = gtk_button_new_with_label("Contrast");
    buton_rotire = gtk_button_new_with_label("Rotire");

    // Adăugarea butoanelor de editare la container
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_grayscale, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_invert, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_detectare_obiect, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_decupare, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_gamma, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_ascutire, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_vizualizare, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_salvare, FALSE, FALSE, 0); // Adăugarea butonului de salvare
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_resetare, FALSE, FALSE, 0); // Adăugarea butonului de resetare
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_estompare, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_sepia, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_margini, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_luminozitate, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_contrast, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_rotire, FALSE, FALSE, 0);

    // Ascunderea butoanelor de editare inițial
    gtk_widget_hide(buton_grayscale);
    gtk_widget_hide(buton_invert);
    gtk_widget_hide(buton_detectare_obiect);
    gtk_widget_hide(buton_decupare);
    gtk_widget_hide(buton_gamma);
    gtk_widget_hide(buton_ascutire);
    gtk_widget_hide(buton_vizualizare);
    gtk_widget_hide(buton_salvare); // Ascundeți butonul de salvare inițial
    gtk_widget_hide(buton_resetare); // Ascundeți butonul de resetare inițial
    gtk_widget_hide(buton_estompare);
    gtk_widget_hide(buton_sepia);
    gtk_widget_hide(buton_margini);
    gtk_widget_hide(buton_luminozitate);
    gtk_widget_hide(buton_contrast);
    gtk_widget_hide(buton_rotire);

    // Conectarea funcțiilor callback la semnalul "clicked" al butoanelor de editare
    g_signal_connect(buton_grayscale, "clicked", G_CALLBACK(pe_buton_grayscale_apasat), NULL);
    g_signal_connect(buton_invert, "clicked", G_CALLBACK(pe_buton_invert_apasat), NULL);
    g_signal_connect(buton_detectare_obiect, "clicked", G_CALLBACK(pe_buton_detectare_obiect_apasat), NULL);
    g_signal_connect(buton_decupare, "clicked", G_CALLBACK(pe_buton_decupare_apasat), NULL);
    g_signal_connect(buton_gamma, "clicked", G_CALLBACK(pe_buton_gamma_apasat), NULL);
    g_signal_connect(buton_ascutire, "clicked", G_CALLBACK(pe_buton_ascutire_apasat), NULL);
    g_signal_connect(buton_vizualizare, "clicked", G_CALLBACK(pe_buton_vizualizare_apasat), NULL);
    g_signal_connect(buton_salvare, "clicked", G_CALLBACK(pe_buton_salvare_apasat), NULL); // Conectarea funcției callback pentru butonul de salvare
    g_signal_connect(buton_resetare, "clicked", G_CALLBACK(pe_buton_resetare_apasat), NULL); // Conectarea funcției callback pentru butonul de resetare
    g_signal_connect(buton_estompare, "clicked", G_CALLBACK(pe_buton_estompare_apasat), NULL);
    g_signal_connect(buton_sepia, "clicked", G_CALLBACK(pe_buton_sepia_apasat), NULL);
    g_signal_connect(buton_margini, "clicked", G_CALLBACK(pe_buton_margini_apasat), NULL);
    g_signal_connect(buton_luminozitate, "clicked", G_CALLBACK(pe_buton_luminozitate_apasat), NULL);
    g_signal_connect(buton_contrast, "clicked", G_CALLBACK(pe_buton_contrast_apasat), NULL);
    g_signal_connect(buton_rotire, "clicked", G_CALLBACK(pe_buton_rotire_apasat), NULL);

    // Crearea butonului de selectare a unei noi imagini
    buton_selecteaza_noua_imagine = gtk_button_new_with_label("Selectează o nouă imagine");
    gtk_widget_set_size_request(buton_selecteaza_noua_imagine, 150, 50);
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_selecteaza_noua_imagine, FALSE, FALSE, 0);
    gtk_widget_hide(buton_selecteaza_noua_imagine); // Ascundeți butonul inițial
    g_signal_connect(buton_selecteaza_noua_imagine, "clicked", G_CALLBACK(pe_buton_selecteaza_noua_imagine_apasat), NULL);

    // Crearea butonului de deconectare
    buton_deconectare = gtk_button_new_with_label("Deconectare");
    gtk_widget_set_size_request(buton_deconectare, 150, 50); // Dimensiunile butonului
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, "button { background-color: red; color: white; }", -1, NULL);
    GtkStyleContext *context = gtk_widget_get_style_context(buton_deconectare);
    gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
    gtk_box_pack_start(GTK_BOX(caseta_butoane), buton_deconectare, FALSE, FALSE, 0);
    gtk_widget_hide(buton_deconectare); // Ascundeți butonul inițial
    g_signal_connect(buton_deconectare, "clicked", G_CALLBACK(pe_buton_deconectare_apasat), NULL);

    // Crearea ferestrelor de logare și înregistrare
    creeaza_fereastra_logare();
    creeaza_fereastra_inregistrare();

    // Ascunderea ferestrei principale până la autentificare
    gtk_widget_hide(fereastra_principala);

    // Intrarea în bucla evenimentelor GTK
    gtk_main();
}

int main(int argc, char *argv[]) {
    creeaza_interfata(argc, argv);
    return 0;
}

