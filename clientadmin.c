/*
    Rulare
    g++ clientadmin.c -o clientadmin `pkg-config --cflags --libs gtk+-3.0 opencv4`
    ./clientadmin
*/
#include <gtk/gtk.h>  // GTK pentru a crea interfața grafică
#include <stdio.h>  // funcții de intrare/ieșire
#include <stdlib.h>  // alocare și eliberare a memoriei
#include <string.h>  // operații cu șiruri de caractere

#define MAX_BUFFER_SIZE 1024  // Definim dimensiunea maximă a buffer-ului

GtkWidget *lista_utilizatori;  // Declaram un obiect pentru lista de utilizatori
GtkWidget *buton_banare;  // Declaram un obiect pentru butonul de banare
GtkWidget *fereastra_principala;  // Declaram un obiect pentru fereastra principala

// Definim structura pentru a reprezenta un utilizator
typedef struct {
    char utilizator[256];
    char parola[256];
    char ban_status[4];
} Utilizator;

// Funcția pentru a citi toți utilizatorii din fișierul users.txt
Utilizator* citeste_utilizatori(int *numar_utilizatori) {
    FILE *file = fopen("users.txt", "r");  // Deschidem fișierul pentru citire
    if (file == NULL) {  // Verificam daca fișierul s-a deschis cu succes
        perror("Eroare la deschiderea fișierului de utilizatori");  // Afisam eroare daca deschiderea a esuat
        return NULL;  // Returnam NULL pentru a indica eroarea
    }

    Utilizator *utilizatori = (Utilizator*)malloc(sizeof(Utilizator) * MAX_BUFFER_SIZE);  // Alocam memorie pentru lista de utilizatori
    *numar_utilizatori = 0;  // Initializam numarul de utilizatori cu 0

    // Citim utilizatorii din fișier până la sfârșitul fișierului
    while (fscanf(file, "%255s %255s %3s", utilizatori[*numar_utilizatori].utilizator, utilizatori[*numar_utilizatori].parola, utilizatori[*numar_utilizatori].ban_status) != EOF) {
        (*numar_utilizatori)++;  // Incrementam numarul de utilizatori citiți
        if (*numar_utilizatori >= MAX_BUFFER_SIZE) {  // Verificam dacă s-a atins limita maximă de utilizatori
            break;  // Ieșim din buclă dacă s-a atins limita
        }
    }

    fclose(file);  // Inchidem fișierul
    return utilizatori;  // Returnam lista de utilizatori citiți
}

// Funcția pentru a actualiza fișierul users.txt după banare
void actualizeaza_fisier_utilizatori(Utilizator *utilizatori, int numar_utilizatori) {
    FILE *file = fopen("users.txt", "w");  // Deschidem fișierul pentru scriere (acesta va fi golit)
    if (file == NULL) {  // Verificam daca fișierul s-a deschis cu succes
        perror("Eroare la deschiderea fișierului de utilizatori");  // Afisam eroare daca deschiderea a esuat
        return;  // Ieșim din funcție
    }

    // Scriem utilizatorii în fișier
    for (int i = 0; i < numar_utilizatori; i++) {
        fprintf(file, "%s %s %s\n", utilizatori[i].utilizator, utilizatori[i].parola, utilizatori[i].ban_status);
    }

    fclose(file);  // Inchidem fișierul
}

// Funcția callback pentru banarea utilizatorului selectat
void pe_buton_banare_apasat(GtkWidget *widget, gpointer data) {
    GtkTreeIter iter;
    GtkTreeModel *model;
    char *utilizator;

    if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(data), &model, &iter)) {  // Verificam daca un utilizator este selectat
        gtk_tree_model_get(model, &iter, 0, &utilizator, -1);  // Obținem numele utilizatorului selectat

        // Citim utilizatorii din fișier
        int numar_utilizatori;
        Utilizator *utilizatori = citeste_utilizatori(&numar_utilizatori);

        // Marcam utilizatorul selectat ca banat
        for (int i = 0; i < numar_utilizatori; i++) {
            if (strcmp(utilizatori[i].utilizator, utilizator) == 0) {  // Verificam daca am gasit utilizatorul selectat
                strcpy(utilizatori[i].ban_status, "ban");  // Marcam utilizatorul ca fiind banat
                break;  // Ieșim din buclă după găsirea utilizatorului
            }
        }

        // Actualizăm fișierul users.txt
        actualizeaza_fisier_utilizatori(utilizatori, numar_utilizatori);

        // Eliberăm memoria
        free(utilizatori);

        // Reîncărcăm lista de utilizatori
        GtkListStore *store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(lista_utilizatori)));
        gtk_list_store_clear(store);  // Golim lista de utilizatori pentru a o reîncărca

        utilizatori = citeste_utilizatori(&numar_utilizatori);  // Citim din nou utilizatorii
        for (int i = 0; i < numar_utilizatori; i++) {  // Iteram prin utilizatori și îi adăugăm în lista de utilizatori
            GtkTreeIter iter;
            gtk_list_store_append(store, &iter);  // Adăugăm un nou utilizator în lista de utilizatori
            gtk_list_store_set(store, &iter, 0, utilizatori[i].utilizator, 1, utilizatori[i].parola, 2, utilizatori[i].ban_status, -1);  // Setăm valorile pentru coloanele listei
        }

        free(utilizatori);  // Eliberăm memoria pentru lista de utilizatori
    }
}

// Funcția pentru a crea interfața grafică
void creeaza_interfata(int argc, char *argv[]) {
    gtk_init(&argc, &argv);  // Inițializăm GTK

    fereastra_principala = gtk_window_new(GTK_WINDOW_TOPLEVEL);  // Creăm fereastra principală
    gtk_window_set_title(GTK_WINDOW(fereastra_principala), "Admin Client");  // Setăm titlul ferestrei
    gtk_container_set_border_width(GTK_CONTAINER(fereastra_principala), 10);  // Setăm marginea ferestrei
    gtk_widget_set_size_request(fereastra_principala, 400, 300);  // Setăm dimensiunea ferestrei
    g_signal_connect(fereastra_principala, "destroy", G_CALLBACK(gtk_main_quit), NULL);  // Conectăm semnalul de închidere a ferestrei cu funcția gtk_main_quit

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);  // Creăm un container de tip cutie verticală
    gtk_container_add(GTK_CONTAINER(fereastra_principala), box);  // Adăugăm containerul în fereastra principală

    // Crearea listei de utilizatori
    GtkListStore *store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);  // Creăm un magazin de liste cu 3 coloane de tip șir de caractere
    lista_utilizatori = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));  // Creăm o nouă listă de vizualizare cu modelul de date dat

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();  // Creăm un renderer pentru text
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Utilizator", renderer, "text", 0, NULL);  // Creăm o coloană pentru utilizatori
    gtk_tree_view_append_column(GTK_TREE_VIEW(lista_utilizatori), column);  // Adăugăm coloana în lista de utilizatori

    column = gtk_tree_view_column_new_with_attributes("Parola", renderer, "text", 1, NULL);  // Creăm o coloană pentru parole
    gtk_tree_view_append_column(GTK_TREE_VIEW(lista_utilizatori), column);  // Adăugăm coloana în lista de utilizatori

    column = gtk_tree_view_column_new_with_attributes("Status Ban", renderer, "text", 2, NULL);  // Creăm o coloană pentru statusul de banare
    gtk_tree_view_append_column(GTK_TREE_VIEW(lista_utilizatori), column);  // Adăugăm coloana în lista de utilizatori

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);  // Creăm o fereastră cu scroll
    gtk_container_add(GTK_CONTAINER(scroll), lista_utilizatori);  // Adăugăm lista de utilizatori în fereastra cu scroll
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);  // Adăugăm fereastra cu scroll în cutia verticală

    // Crearea butonului de banare
    buton_banare = gtk_button_new_with_label("Banează utilizator");  // Creăm un buton cu eticheta "Banează utilizator"
    gtk_box_pack_start(GTK_BOX(box), buton_banare, FALSE, FALSE, 0);  // Adăugăm butonul de banare în cutia verticală

    // Conectarea funcției callback pentru butonul de banare
    GtkTreeSelection *selectie = gtk_tree_view_get_selection(GTK_TREE_VIEW(lista_utilizatori));  // Obținem selecția din lista de utilizatori
    g_signal_connect(buton_banare, "clicked", G_CALLBACK(pe_buton_banare_apasat), selectie);  // Conectăm semnalul de click al butonului cu funcția de banare

    // Încărcarea utilizatorilor în lista de utilizatori
    int numar_utilizatori;
    Utilizator *utilizatori = citeste_utilizatori(&numar_utilizatori);  // Citim utilizatorii din fișier

    for (int i = 0; i < numar_utilizatori; i++) {  // Iterăm prin utilizatori și îi adăugăm în lista de utilizatori
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);  // Adăugăm un nou utilizator în lista de utilizatori
        gtk_list_store_set(store, &iter, 0, utilizatori[i].utilizator, 1, utilizatori[i].parola, 2, utilizatori[i].ban_status, -1);  // Setăm valorile pentru coloanele listei
    }

    free(utilizatori);  // Eliberăm memoria pentru lista de utilizatori

    gtk_widget_show_all(fereastra_principala);  // Arătăm toate widget-urile din fereastra principală
    gtk_main();  // Intrăm în bucla principală GTK
}

int main(int argc, char *argv[]) {
    creeaza_interfata(argc, argv);  // Apelăm funcția pentru a crea interfața grafică
    return 0;  // Ieșim din program cu cod de succes
}
