#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <mariadb/mysql.h>
#include <gtk/gtk.h>
#include <unistd.h>

// Add error handling macros
#define CHECK_MYSQL_ERROR(conn, msg) do { \
    if (mysql_errno(conn)) { \
        fprintf(stderr, "%s: %s\n", msg, mysql_error(conn)); \
        return; \
    } \
} while(0)

#define CHECK_MYSQL_ERROR_RETURN(conn, msg, ret) do { \
    if (mysql_errno(conn)) { \
        fprintf(stderr, "%s: %s\n", msg, mysql_error(conn)); \
        return ret; \
    } \
} while(0)

#define MAX_QUERY_LEN 512
#define LIB_NAME_LEN 100
#define TITLE_LEN 100
#define ISBN_LEN 20
#define GENRE_LEN 50
#define YEAR_LEN 10
#define SHELF_LEN 30
#define ADDRESS_LEN 200
#define PHONE_LEN 20
#define EMAIL_LEN 100
#define ROLE_LEN 50
#define STATUS_LEN 20
#define DATE_LEN 11
#define BIO_LEN 1000
#define PASSWORD_LEN 50
#define CONTACT_LEN 100

MYSQL *conn;
GtkWidget *main_window;
int current_user_id = -1;
char current_user_role[20] = "";

// Structure definitions
typedef struct {
    GtkWidget *book_id;
    GtkWidget *title;
    GtkWidget *author_id;
    GtkWidget *publisher_id;
    GtkWidget *isbn;
    GtkWidget *genre;
    GtkWidget *year;
    GtkWidget *copies;
    GtkWidget *shelf;
} BookEntryWidgets;

typedef struct {
    GtkWidget *username;
    GtkWidget *password;
} LoginWidgets;

// Additional structure definitions
typedef struct {
    GtkWidget *author_id;
    GtkWidget *name;
    GtkWidget *bio;
} AuthorEntryWidgets;

typedef struct {
    GtkWidget *username;
    GtkWidget *password;
    GtkWidget *name;
    GtkWidget *email;
    GtkWidget *phone;
    GtkWidget *address;
    GtkWidget *role;
} RegistrationWidgets;

// Forward declarations
void createDashboardWindow(void);
void insertBookGUI(GtkWidget *widget, gpointer data);
void on_borrow_button_clicked(GtkWidget *widget, gpointer data);
void on_gui_login_clicked(GtkWidget *widget, gpointer data);
void connectDB(void);
void showMessage(GtkWindow *parent, GtkMessageType type, const char *msg);
int isValidInteger(const char *str);
int isNonEmptyString(const char *str, int max_len);
int isValidYear(const char *str);
int isValidDate(const char *str);
int idExistsInTable(const char *table, const char *column, const char *id);
void createInsertBookWindow(void);
void viewBooksGUI(GtkWidget *widget, gpointer parent_window);
int authenticateUser(const char *username, const char *password, int *user_id, char *user_role);
void runGuiLogin(int argc, char *argv[]);
void runTerminalInterface(void);
void on_gui_login_clicked(GtkWidget *widget, gpointer data);
int terminalLogin(void);
void createInsertAuthorWindow(void);
void viewAuthorsGUI(GtkWidget *widget, gpointer parent_window);
void showPublicRegistrationWindow(void);
void showStaffRegistrationWindow(void);
void createPublicRegistrationWindow(void);
void createStaffRegistrationWindow(void);
void createInsertPublisherWindow(void);
void viewPublishersGUI(GtkWidget *widget, gpointer parent_window);
void createInsertMemberWindow(void);
void viewMembersGUI(GtkWidget *widget, gpointer parent_window);
void createInsertBorrowingWindow(void);
void viewBorrowingsGUI(GtkWidget *widget, gpointer parent_window);
void createInsertFineWindow(void);
void viewFinesGUI(GtkWidget *widget, gpointer parent_window);
void insertPublisherGUI(GtkWidget *widget, gpointer data);
void insertMemberGUI(GtkWidget *widget, gpointer data);
void insertBorrowingGUI(GtkWidget *widget, gpointer data);
void insertFineGUI(GtkWidget *widget, gpointer data);
void insertStaffGUI(GtkWidget *widget, gpointer data);
static void registerPublicUser(GtkWidget *widget, gpointer data);
static void registerStaffUser(GtkWidget *widget, gpointer data);
void showMessage(GtkWindow *parent, GtkMessageType type, const char *message) {
    GtkWidget *dialog;
    dialog = gtk_message_dialog_new(parent,
                                    GTK_DIALOG_DESTROY_WITH_PARENT,
                                    type,
                                    GTK_BUTTONS_OK,
                                    "%s", message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}
int registerMember(const char *username, const char *password, const char *name, 
                const char *email, const char *phone, const char *address, 
                int *member_id);
int registerStaff(const char *username, const char *password, const char *name, 
                const char *email, const char *phone, const char *address, 
                const char *role, int *staff_id);

void insertStaffGUI(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const gchar *name = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const gchar *role = gtk_entry_get_text(GTK_ENTRY(entries[1]));
    const gchar *email = gtk_entry_get_text(GTK_ENTRY(entries[2]));
    const gchar *phone = gtk_entry_get_text(GTK_ENTRY(entries[3]));
    const gchar *password = gtk_entry_get_text(GTK_ENTRY(entries[4]));

    char query[512];
    snprintf(query, sizeof(query),
             "INSERT INTO Staff(name, role, email, phone,password) VALUES('%s', '%s', '%s', '%s')",
             name, role, email, phone);

    if (mysql_query(conn, query)) {
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)),GTK_MESSAGE_INFO,"Failed to insert staff.");
    } else {
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)),GTK_MESSAGE_INFO,"Staff inserted successfully.");
    }
}

void createPublicRegistrationWindow(void) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Public Registration");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);

    const char *labels[] = {"Username:", "Password:", "Name:", "Email:", 
                           "Phone:", "Address:"};
    GtkWidget *entries[6];
    
    for (int i = 0; i < 6; i++) {
        GtkWidget *label = gtk_label_new(labels[i]);
        entries[i] = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(grid), label, 0, i, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entries[i], 1, i, 1, 1);
    }

    // Make password entry hidden
    gtk_entry_set_visibility(GTK_ENTRY(entries[1]), FALSE);

    GtkWidget *submit = gtk_button_new_with_label("Register");
    gtk_grid_attach(GTK_GRID(grid), submit, 0, 6, 2, 1);

    // Connect the submit button to the callback function
    g_signal_connect(submit, "clicked", G_CALLBACK(registerPublicUser), entries);
    
    // Connect the window's destroy signal to free the entries array
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_widget_destroy), NULL);
    
    gtk_widget_show_all(window);
}

void createInsertPublisherWindow(void) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Insert New Publisher");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 150);

    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

    GtkWidget *label_id = gtk_label_new("Publisher ID:");
    GtkWidget *entry_id = gtk_entry_new();
    GtkWidget *label_name = gtk_label_new("Name:");
    GtkWidget *entry_name = gtk_entry_new();
    GtkWidget *insert_button = gtk_button_new_with_label("Insert Publisher");

    gtk_grid_attach(GTK_GRID(grid), label_id, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_id, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label_name, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_name, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), insert_button, 1, 2, 1, 1);

    GtkWidget *entries[2] = {entry_id, entry_name};
    g_signal_connect(insert_button, "clicked", G_CALLBACK(insertPublisherGUI), entries);
    
    gtk_widget_show_all(window);
}

void viewPublishersGUI(GtkWidget *widget, gpointer parent_window) {
    GtkTreeIter iter;
    GtkWidget *view_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(view_window), "View Publishers");
    gtk_window_set_default_size(GTK_WINDOW(view_window), 400, 300);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(view_window), scrolled_window);

    GtkListStore *store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree);

    const char *titles[] = {"Publisher ID", "Name"};
    for (int i = 0; i < 2; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(titles[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    }

    char query[] = "SELECT publisher_id, name FROM Publishers";
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *res = mysql_store_result(conn);
        if (res) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, 0, row[0], 1, row[1], -1);
            }
            mysql_free_result(res);
        }
    }

    gtk_widget_show_all(view_window);
}

void createInsertMemberWindow(void) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Insert New Member");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 350);

    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);

    const char *labels[] = {"Username:", "Password:", "Name:", "Email:", 
                           "Phone:", "Address:"};
    GtkWidget *entries[6];
    
    for (int i = 0; i < 6; i++) {
        GtkWidget *label = gtk_label_new(labels[i]);
        entries[i] = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(grid), label, 0, i, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entries[i], 1, i, 1, 1);
    }

    // Make password entry hidden
    gtk_entry_set_visibility(GTK_ENTRY(entries[1]), FALSE);

    GtkWidget *insert_button = gtk_button_new_with_label("Insert Member");
    gtk_grid_attach(GTK_GRID(grid), insert_button, 0, 6, 2, 1);

    g_signal_connect(insert_button, "clicked", G_CALLBACK(insertMemberGUI), entries);
    gtk_widget_show_all(window);
}

void viewMembersGUI(GtkWidget *widget, gpointer parent_window) {
    GtkTreeIter iter;
    GtkWidget *view_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(view_window), "View Members");
    gtk_window_set_default_size(GTK_WINDOW(view_window), 600, 400);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(view_window), scrolled_window);

    // Assuming Members table has columns: member_id, username, name, email, phone, address
    GtkListStore *store = gtk_list_store_new(6, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree);

    const char *titles[] = {"Member ID", "Username", "Name", "Email", "Phone", "Address"};
    for (int i = 0; i < 6; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(titles[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    }

    char query[] = "SELECT id,name, email, phone, address FROM Members";
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *res = mysql_store_result(conn);
        if (res) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, 0, row[0], 1, row[1], 2, row[2], 3, row[3], 4, row[4], 5, row[5], -1);
            }
            mysql_free_result(res);
        }
    }

    gtk_widget_show_all(view_window);
}

void insertBookGUI(GtkWidget *widget, gpointer data) {
    BookEntryWidgets *entries = (BookEntryWidgets *)data;

    const char *book_id = gtk_entry_get_text(GTK_ENTRY(entries->book_id));
    const char *title = gtk_entry_get_text(GTK_ENTRY(entries->title));
    const char *author_id = gtk_entry_get_text(GTK_ENTRY(entries->author_id));
    const char *publisher_id = gtk_entry_get_text(GTK_ENTRY(entries->publisher_id));
    const char *isbn = gtk_entry_get_text(GTK_ENTRY(entries->isbn));
    const char *genre = gtk_entry_get_text(GTK_ENTRY(entries->genre));
    const char *year = gtk_entry_get_text(GTK_ENTRY(entries->year));
    const char *copies = gtk_entry_get_text(GTK_ENTRY(entries->copies));
    const char *shelf = gtk_entry_get_text(GTK_ENTRY(entries->shelf));

    GtkWindow *parent_window = GTK_WINDOW(gtk_widget_get_toplevel(widget));

    if (strlen(book_id) == 0 || strlen(title) == 0) {
        showMessage(parent_window, GTK_MESSAGE_ERROR, "Book ID and Title are required.");
        return;
    }

    // Prepare SQL insert statement
    char query[1024];
    snprintf(query, sizeof(query),
        "INSERT INTO Books (book_id, title, author_id, publisher_id, isbn, genre, year_published, copies_available, shelf_location) "
        "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')",
        book_id, title, author_id, publisher_id, isbn, genre, year, copies, shelf);
    // Execute the query using your DB connection (assuming `db` is available)
    if (mysql_query(conn, query)) {
        showMessage(parent_window, GTK_MESSAGE_ERROR, mysql_error(conn));
        return;
    }

    showMessage(parent_window, GTK_MESSAGE_INFO, "Book inserted successfully.");
}
void createInsertBorrowingWindow(void) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Insert New Borrowing");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);

    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

    const char *labels[] = {"Book ID:", "Member ID:", "Borrow Date (YYYY-MM-DD):"};
    GtkWidget *entries[3];
    
    for (int i = 0; i < 3; i++) {
        GtkWidget *label = gtk_label_new(labels[i]);
        entries[i] = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(grid), label, 0, i, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entries[i], 1, i, 1, 1);
    }

    GtkWidget *insert_button = gtk_button_new_with_label("Insert Borrowing");
    gtk_grid_attach(GTK_GRID(grid), insert_button, 1, 3, 1, 1);

    g_signal_connect(insert_button, "clicked", G_CALLBACK(insertBorrowingGUI), entries);
    
    gtk_widget_show_all(window);
}

void viewBorrowingsGUI(GtkWidget *widget, gpointer parent_window) {
    GtkTreeIter iter;
    GtkWidget *view_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(view_window), "View Borrowings");
    gtk_window_set_default_size(GTK_WINDOW(view_window), 600, 400);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(view_window), scrolled_window);

    // Assuming Borrowings table has columns: borrowing_id, book_id, member_id, borrow_date, return_date
    GtkListStore *store = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree);

    const char *titles[] = {"Borrowing ID", "Book ID", "Member ID", "Borrow Date", "Return Date"};
    for (int i = 0; i < 5; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(titles[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    }

    char query[] = "SELECT borrowing_id, book_id, member_id, borrow_date, return_date FROM Borrowings";
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *res = mysql_store_result(conn);
        if (res) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, 0, row[0], 1, row[1], 2, row[2], 3, row[3], 4, row[4] ? row[4] : "N/A", -1);
            }
            mysql_free_result(res);
        }
    }

    gtk_widget_show_all(view_window);
}

void createInsertFineWindow(void) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Insert New Fine");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 250);

    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

    const char *labels[] = {"Borrowing ID:", "Fine Amount:", "Fine Date (YYYY-MM-DD):", "Status:"};
    GtkWidget *entries[4];
    
    for (int i = 0; i < 4; i++) {
        GtkWidget *label = gtk_label_new(labels[i]);
        entries[i] = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(grid), label, 0, i, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entries[i], 1, i, 1, 1);
    }

    GtkWidget *insert_button = gtk_button_new_with_label("Insert Fine");
    gtk_grid_attach(GTK_GRID(grid), insert_button, 1, 4, 1, 1);

    g_signal_connect(insert_button, "clicked", G_CALLBACK(insertFineGUI), entries);
    
    gtk_widget_show_all(window);
}

void insertPublisherGUI(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const char *publisher_id = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char *name = gtk_entry_get_text(GTK_ENTRY(entries[1]));

    if (!isValidInteger(publisher_id)) {
        showMessage(NULL, GTK_MESSAGE_ERROR, "Publisher ID must be a positive integer.");
        return;
    }
    if (!isNonEmptyString(name, LIB_NAME_LEN)) {
        showMessage(NULL, GTK_MESSAGE_ERROR, "Name must be a non-empty string.");
        return;
    }

    char escaped_name[LIB_NAME_LEN * 2];
    mysql_real_escape_string(conn, escaped_name, name, strlen(name));

    char query[MAX_QUERY_LEN];
    snprintf(query, sizeof(query),
             "INSERT INTO Publishers (publisher_id, name) VALUES (%s, '%s')",
             publisher_id, escaped_name);

    if (mysql_query(conn, query)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Failed to insert publisher: %s", mysql_error(conn));
        showMessage(NULL, GTK_MESSAGE_ERROR, error_msg);
    } else {
        showMessage(NULL, GTK_MESSAGE_INFO, "Publisher inserted successfully.");
    }
}

void insertMemberGUI(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const char *username = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char *password = gtk_entry_get_text(GTK_ENTRY(entries[1]));
    const char *name = gtk_entry_get_text(GTK_ENTRY(entries[2]));
    const char *email = gtk_entry_get_text(GTK_ENTRY(entries[3]));
    const char *phone = gtk_entry_get_text(GTK_ENTRY(entries[4]));
    const char *address = gtk_entry_get_text(GTK_ENTRY(entries[5]));

    GtkWindow *parent_window = GTK_WINDOW(gtk_widget_get_toplevel(widget));

    // Basic validation
     if (!isNonEmptyString(username, LIB_NAME_LEN) || !isNonEmptyString(password, PASSWORD_LEN) ||
        !isNonEmptyString(name, LIB_NAME_LEN) || !isNonEmptyString(email, EMAIL_LEN) ||
        !isNonEmptyString(phone, PHONE_LEN) || !isNonEmptyString(address, ADDRESS_LEN)) {
        showMessage(parent_window, GTK_MESSAGE_ERROR, 
                   "All fields must be filled out.");
        return;
    }

    char escaped_username[LIB_NAME_LEN * 2];
    char escaped_password[PASSWORD_LEN * 2];
    char escaped_name[LIB_NAME_LEN * 2];
    char escaped_email[EMAIL_LEN * 2];
    char escaped_phone[PHONE_LEN * 2];
    char escaped_address[ADDRESS_LEN * 2];

    mysql_real_escape_string(conn, escaped_username, username, strlen(username));
    mysql_real_escape_string(conn, escaped_password, password, strlen(password));
    mysql_real_escape_string(conn, escaped_name, name, strlen(name));
    mysql_real_escape_string(conn, escaped_email, email, strlen(email));
    mysql_real_escape_string(conn, escaped_phone, phone, strlen(phone));
    mysql_real_escape_string(conn, escaped_address, address, strlen(address));

    char query[MAX_QUERY_LEN];
    snprintf(query, sizeof(query),
             "INSERT INTO Members (password, name, email, phone, address) " 
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s')",
             escaped_username, escaped_password, escaped_name, escaped_email,
             escaped_phone, escaped_address);

    if (mysql_query(conn, query)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Failed to insert member: %s", mysql_error(conn));
        showMessage(parent_window, GTK_MESSAGE_ERROR, error_msg);
    } else {
        showMessage(parent_window, GTK_MESSAGE_INFO, "Member inserted successfully.");
        // Optionally clear the entry fields or close the window
    }
}

void insertBorrowingGUI(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const char *book_id_str = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char *member_id_str = gtk_entry_get_text(GTK_ENTRY(entries[1]));
    const char *borrow_date = gtk_entry_get_text(GTK_ENTRY(entries[2]));
    
    GtkWindow *parent_window = GTK_WINDOW(gtk_widget_get_toplevel(widget));

    if (!isValidInteger(book_id_str) || !isValidInteger(member_id_str) || !isValidDate(borrow_date)) {
        showMessage(parent_window, GTK_MESSAGE_ERROR, "Invalid Book ID, Member ID (must be positive integers), or Borrow Date format (YYYY-MM-DD).");
        return;
    }

    int book_id = atoi(book_id_str);
    int member_id = atoi(member_id_str);

    // Check if book exists and has copies available
    char check_book_query[MAX_QUERY_LEN];
    snprintf(check_book_query, sizeof(check_book_query),
             "SELECT copies_available FROM Books WHERE book_id = %d", book_id);
    if (mysql_query(conn, check_book_query)) {
        showMessage(parent_window, GTK_MESSAGE_ERROR, "Error checking book availability.");
        return;
    }
    MYSQL_RES *book_res = mysql_store_result(conn);
    if (!book_res || mysql_num_rows(book_res) == 0) {
        showMessage(parent_window, GTK_MESSAGE_ERROR, "Book not found.");
        if (book_res) mysql_free_result(book_res);
        return;
    }
    MYSQL_ROW book_row = mysql_fetch_row(book_res);
    int copies_available = atoi(book_row[0]);
    mysql_free_result(book_res);

     if (copies_available <= 0) {
        showMessage(parent_window, GTK_MESSAGE_INFO, "No copies of this book are currently available.");
        return;
    }

    // Check if member_id exists in Members table
    if (!idExistsInTable("Members", "member_id", member_id_str)) {
         showMessage(parent_window, GTK_MESSAGE_ERROR, "Member not found.");
         return;
    }

    // Record the borrowing
    char borrow_query[MAX_QUERY_LEN];
    snprintf(borrow_query, sizeof(borrow_query),
             "INSERT INTO Borrowings (book_id, member_id, borrow_date) VALUES (%d, %d, '%s')",
             book_id, member_id, borrow_date);

    if (mysql_query(conn, borrow_query)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Error recording borrowing: %s", mysql_error(conn));
        showMessage(parent_window, GTK_MESSAGE_ERROR, error_msg);
        return;
    }

    // Decrease available copies in Books table
    char update_query[MAX_QUERY_LEN];
    snprintf(update_query, sizeof(update_query),
             "UPDATE Books SET copies_available = copies_available - 1 WHERE book_id = %d", book_id);

    if (mysql_query(conn, update_query)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Error updating book copies: %s", mysql_error(conn));
        showMessage(parent_window, GTK_MESSAGE_ERROR, error_msg);
        // Consider rolling back the insert if the update fails
    } else {
        showMessage(parent_window, GTK_MESSAGE_INFO, "Borrowing recorded successfully!");
        // Optionally clear the entry fields or close the window
    }
}

void insertFineGUI(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const char *borrowing_id_str = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char *fine_amount_str = gtk_entry_get_text(GTK_ENTRY(entries[1]));
    const char *fine_date = gtk_entry_get_text(GTK_ENTRY(entries[2]));
    const char *status = gtk_entry_get_text(GTK_ENTRY(entries[3]));

    GtkWindow *parent_window = GTK_WINDOW(gtk_widget_get_toplevel(widget));

    if (!isValidInteger(borrowing_id_str)) {
        showMessage(parent_window, GTK_MESSAGE_ERROR, "Borrowing ID must be a positive integer.");
        return;
    }
    int borrowing_id = atoi(borrowing_id_str);

    double fine_amount = atof(fine_amount_str);
    if (fine_amount <= 0) {
        showMessage(parent_window, GTK_MESSAGE_ERROR, "Fine amount must be a positive number.");
        return;
    }

    if (!isValidDate(fine_date)) {
        showMessage(parent_window, GTK_MESSAGE_ERROR, "Invalid Fine Date format (YYYY-MM-DD).");
        return;
    }
    if (!isNonEmptyString(status, STATUS_LEN)) {
         showMessage(parent_window, GTK_MESSAGE_ERROR, "Status must be a non-empty string (max 20 chars).");
        return;
    }
    
    // Check if borrowing_id exists in Borrowings table
    if (!idExistsInTable("Borrowings", "borrowing_id", borrowing_id_str)) {
         showMessage(parent_window, GTK_MESSAGE_ERROR, "Borrowing not found.");
         return;
    }

    char escaped_status[STATUS_LEN * 2];
    mysql_real_escape_string(conn, escaped_status, status, strlen(status));

    char query[MAX_QUERY_LEN];
    snprintf(query, sizeof(query), "INSERT INTO Fines (borrowing_id, fine_amount, fine_date, status) VALUES (%d, %.2f, '%s', '%s')",
             borrowing_id, fine_amount, fine_date, escaped_status);

    if (mysql_query(conn, query)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Error recording fine: %s", mysql_error(conn));
        showMessage(parent_window, GTK_MESSAGE_ERROR, error_msg);
    } else {
        showMessage(parent_window, GTK_MESSAGE_INFO, "Fine recorded successfully.");
        // Optionally clear the entry fields or close the window
    }
}

void createInsertStaffWindow(void) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Insert Staff");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    const gchar *labels[] = {"Name", "Role", "Email", "Phone"};
    GtkWidget *entries[4];

    for (int i = 0; i < 4; i++) {
        GtkWidget *label = gtk_label_new(labels[i]);
        entries[i] = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(grid), label, 0, i, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entries[i], 1, i, 1, 1);
    }

    GtkWidget *submit = gtk_button_new_with_label("Submit");
    g_signal_connect(submit, "clicked", G_CALLBACK(insertStaffGUI), entries);
    gtk_grid_attach(GTK_GRID(grid), submit, 0, 4, 2, 1);

    gtk_widget_show_all(window);
}

void viewStaffGUI(GtkWidget *widget, gpointer parent_window) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Staff List");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(window), scrolled_window);

    GtkWidget *text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));

    if (mysql_query(conn, "SELECT * FROM Staff")) {
        gtk_text_buffer_set_text(buffer, "Failed to fetch staff data.", -1);
    } else {
        MYSQL_RES *res = mysql_store_result(conn);
        MYSQL_ROW row;
        GString *output = g_string_new(NULL);

        while ((row = mysql_fetch_row(res))) {
            g_string_append_printf(output, "ID: %s\nName: %s\nRole: %s\nEmail: %s\nPhone: %s\n\n",
                                   row[0], row[1], row[2], row[3], row[4]);
        }
        mysql_free_result(res);
        gtk_text_buffer_set_text(buffer, output->str, -1);
        g_string_free(output, TRUE);
    }

    gtk_widget_show_all(window);
}
int isValidInteger(const char *str) {
    if (!str || !*str) return 0;
    for (int i = 0; str[i]; i++) {
        if (!isdigit(str[i])) return 0;
    }
    return atoi(str) > 0;
}

int isNonEmptyString(const char *str, int max_len) {
    return str && *str && strlen(str) <= max_len;
}

int isValidYear(const char *str) {
    if (!isValidInteger(str)) return 0;
    int year = atoi(str);
    return year >= 1800 && year <= 2025;
}

int isValidDate(const char *str) {
    // Expect format: YYYY-MM-DD
    if (strlen(str) != 10 || str[4] != '-' || str[7] != '-') return 0;
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) continue;
        if (!isdigit(str[i])) return 0;
    }
    int year = atoi(str);
    int month = atoi(str + 5);
    int day = atoi(str + 8);
    return year >= 1800 && year <= 2025 && month >= 1 && month <= 12 && day >= 1 && day <= 31;
}

int idExistsInTable(const char *table, const char *column, const char *id) {
    char query[MAX_QUERY_LEN];
    snprintf(query, sizeof(query), "SELECT %s FROM %s WHERE %s = %s", column, table, column, id);
    if (mysql_query(conn, query)) return 0;
    MYSQL_RES *res = mysql_store_result(conn);
    if (!res) return 0;
    int exists = mysql_num_rows(res) > 0;
    mysql_free_result(res);
    return exists;
}

int authenticateUser(const char *username, const char *password, int *user_id, char *user_role) {
    char escaped_username[LIB_NAME_LEN * 2];
    char escaped_password[PASSWORD_LEN * 2];
    
    mysql_real_escape_string(conn, escaped_username, username, strlen(username));
    mysql_real_escape_string(conn, escaped_password, password, strlen(password));

    char query[MAX_QUERY_LEN];
    MYSQL_RES *res = NULL;
    MYSQL_ROW row;

    // Check in Members table
    snprintf(query, sizeof(query), "SELECT member_id FROM Members WHERE name = '%s' AND password = '%s'",
             escaped_username, escaped_password);

    if (mysql_query(conn, query) == 0) {
        res = mysql_store_result(conn);
        if (res && mysql_num_rows(res) > 0) {
            row = mysql_fetch_row(res);
            *user_id = atoi(row[0]);
            strncpy(user_role, "Member", 19);
            user_role[19] = '\0';
            mysql_free_result(res);
            return 1; // Authenticated as Member
        }
        if (res) mysql_free_result(res);
    }

    // Check in Staff table
    snprintf(query, sizeof(query), "SELECT staff_id, role FROM Staff WHERE name = '%s' AND password = '%s'",
             escaped_username, escaped_password);

    if (mysql_query(conn, query) == 0) {
        res = mysql_store_result(conn);
        if (res && mysql_num_rows(res) > 0) {
            row = mysql_fetch_row(res);
            *user_id = atoi(row[0]);

            // Normalize roles from DB (e.g., Librarian → Staff)
            if (strcmp(row[1], "Librarian") == 0) {
                strncpy(user_role, "Staff", 19);
            } else {
                strncpy(user_role, row[1], 19); // e.g., Admin
            }

            user_role[19] = '\0';
            mysql_free_result(res);
            return 1; // Authenticated as Staff/Admin
        }
        if (res) mysql_free_result(res);
    }

    return 0; // Authentication failed
}

int registerMember(const char *username, const char *password, const char *name, 
                const char *email, const char *phone, const char *address, 
                int *member_id) {
    char escaped_username[LIB_NAME_LEN * 2];
    char escaped_password[PASSWORD_LEN * 2];
    char escaped_name[LIB_NAME_LEN * 2];
    char escaped_email[EMAIL_LEN * 2];
    char escaped_phone[PHONE_LEN * 2];
    char escaped_address[ADDRESS_LEN * 2];

    mysql_real_escape_string(conn, escaped_username, username, strlen(username));
    mysql_real_escape_string(conn, escaped_password, password, strlen(password));
    mysql_real_escape_string(conn, escaped_name, name, strlen(name));
    mysql_real_escape_string(conn, escaped_email, email, strlen(email));
    mysql_real_escape_string(conn, escaped_phone, phone, strlen(phone));
    mysql_real_escape_string(conn, escaped_address, address, strlen(address));

    char query[MAX_QUERY_LEN];
    // Assuming Members table has columns: member_id (auto-increment), username, password, name, email, phone, address
    snprintf(query, sizeof(query),
             "INSERT INTO Members (username, password, name, email, phone, address) " 
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s')",
             escaped_username, escaped_password, escaped_name, escaped_email,
             escaped_phone, escaped_address);

    if (mysql_query(conn, query)) {
        // Check for duplicate entry error specifically if needed, otherwise generic fail
        fprintf(stderr, "Error registering member: %s\n", mysql_error(conn));
        return 0;
    }

    *member_id = mysql_insert_id(conn);
    return 1;
}

int registerStaff(const char *username, const char *password, const char *name, 
                const char *email, const char *phone, const char *address, 
                const char *role, int *staff_id) {
    char escaped_username[LIB_NAME_LEN * 2];
    char escaped_password[PASSWORD_LEN * 2];
    char escaped_name[LIB_NAME_LEN * 2];
    char escaped_email[EMAIL_LEN * 2];
    char escaped_phone[PHONE_LEN * 2];
    char escaped_address[ADDRESS_LEN * 2];
    char escaped_role[ROLE_LEN * 2];

    mysql_real_escape_string(conn, escaped_username, username, strlen(username));
    mysql_real_escape_string(conn, escaped_password, password, strlen(password));
    mysql_real_escape_string(conn, escaped_name, name, strlen(name));
    mysql_real_escape_string(conn, escaped_email, email, strlen(email));
    mysql_real_escape_string(conn, escaped_phone, phone, strlen(phone));
    mysql_real_escape_string(conn, escaped_address, address, strlen(address));
    mysql_real_escape_string(conn, escaped_role, role, strlen(role));

    char query[MAX_QUERY_LEN];
    // Assuming Staff table has columns: staff_id (auto-increment), username, password, name, email, phone, address, role
     snprintf(query, sizeof(query), 
             "INSERT INTO Staff (username, password, name, email, phone, address, role) " 
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s')", 
             escaped_username, escaped_password, escaped_name, escaped_email, 
             escaped_phone, escaped_address, escaped_role);

    if (mysql_query(conn, query)) {
        // Check for duplicate entry error specifically if needed, otherwise generic fail
        fprintf(stderr, "Error registering staff: %s\n", mysql_error(conn));
        return 0;
    }

    *staff_id = mysql_insert_id(conn);
    return 1;
}

void createInsertBookWindow(void) {
    GtkWidget *book_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(book_window), "Insert New Book");
    gtk_window_set_default_size(GTK_WINDOW(book_window), 400, 400);

    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(book_window), grid);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

    BookEntryWidgets *widgets = g_new(BookEntryWidgets, 1);
    widgets->book_id = gtk_entry_new();
    widgets->title = gtk_entry_new();
    widgets->author_id = gtk_entry_new();
    widgets->publisher_id = gtk_entry_new();
    widgets->isbn = gtk_entry_new();
    widgets->genre = gtk_entry_new();
    widgets->year = gtk_entry_new();
    widgets->copies = gtk_entry_new();
    widgets->shelf = gtk_entry_new();

    const char *labels[] = {"Book ID:", "Title:", "Author ID:", "Publisher ID:", 
                           "ISBN:", "Genre:", "Year:", "Copies:", "Shelf Location:"};
    GtkWidget *entry_widgets[] = {widgets->book_id, widgets->title, widgets->author_id,
                                 widgets->publisher_id, widgets->isbn, widgets->genre,
                                 widgets->year, widgets->copies, widgets->shelf};

    for (int i = 0; i < 9; i++) {
        GtkWidget *label = gtk_label_new(labels[i]);
        gtk_grid_attach(GTK_GRID(grid), label, 0, i, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entry_widgets[i], 1, i, 1, 1);
    }

    GtkWidget *insert_button = gtk_button_new_with_label("Insert Book");
    gtk_grid_attach(GTK_GRID(grid), insert_button, 1, 9, 1, 1);
    g_signal_connect(insert_button, "clicked", G_CALLBACK(insertBookGUI), widgets);
    
    g_signal_connect_swapped(book_window, "destroy", G_CALLBACK(g_free), widgets);
    gtk_widget_show_all(book_window);
}

void viewBooksGUI(GtkWidget *widget, gpointer parent_window) {
    GtkTreeIter iter;
    GtkWidget *view_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(view_window), "View Books");
    gtk_window_set_default_size(GTK_WINDOW(view_window), 800, 400);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(view_window), scrolled_window);

    GtkListStore *store = gtk_list_store_new(9, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                             G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
                                             G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree);

    const char *titles[] = {"Book ID", "Title", "Author ID", "Publisher ID", "ISBN",
                            "Genre", "Year", "Copies", "Shelf"};
    for (int i = 0; i < 9; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(titles[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    }

    char query[] = "SELECT book_id, title, author_id, publisher_id, isbn, genre, year_published, copies_available, shelf_location FROM Books";
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *res = mysql_store_result(conn);
        if (res) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, 0, row[0], 1, row[1], 2, row[2], 3, row[3],
                                   4, row[4], 5, row[5], 6, row[6], 7, row[7], 8, row[8], -1);
            }
            mysql_free_result(res);
        }
    }

    gtk_widget_show_all(view_window);
}

void displayBooksTable() {
    const char *query =
        "SELECT B.book_id, B.title, A.name AS author_name, B.copies_available "
        "FROM Books B JOIN Authors A ON B.author_id = A.author_id";

    if (mysql_query(conn, query)) {
        printf("Error fetching book list: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (!res) {
        printf("No books found or error reading list.\n");
        return;
    }

    MYSQL_ROW row;
    printf("\n======================== Available Books ========================\n");
    printf("+----------+------------------------------+----------------------+------------+\n");
    printf("| %-8s | %-28s | %-20s | %-10s |\n", "Book ID", "Title", "Author", "Available");
    printf("+----------+------------------------------+----------------------+------------+\n");

    while ((row = mysql_fetch_row(res))) {
        printf("| %-8s | %-28s | %-20s | %-10s |\n", row[0], row[1], row[2], row[3]);
    }

    printf("+----------+------------------------------+----------------------+------------+\n");
    mysql_free_result(res);
}

void insertAuthorGUI(GtkWidget *widget, gpointer data) {
    AuthorEntryWidgets *widgets = (AuthorEntryWidgets *)data;
    
    const char *author_id = gtk_entry_get_text(GTK_ENTRY(widgets->author_id));
    const char *name = gtk_entry_get_text(GTK_ENTRY(widgets->name));
    const char *bio = gtk_entry_get_text(GTK_ENTRY(widgets->bio));

    if (!isValidInteger(author_id)) {
        showMessage(NULL, GTK_MESSAGE_ERROR, "Author ID must be a positive integer.");
        return;
    }
    if (!isNonEmptyString(name, LIB_NAME_LEN)) {
        showMessage(NULL, GTK_MESSAGE_ERROR, "Name must be a non-empty string.");
        return;
    }
    if (!isNonEmptyString(bio, BIO_LEN)) {
        showMessage(NULL, GTK_MESSAGE_ERROR, "Bio must be a non-empty string.");
        return;
    }

    char escaped_name[LIB_NAME_LEN * 2];
    char escaped_bio[BIO_LEN * 2];
    mysql_real_escape_string(conn, escaped_name, name, strlen(name));
    mysql_real_escape_string(conn, escaped_bio, bio, strlen(bio));

    char query[MAX_QUERY_LEN];
    snprintf(query, sizeof(query),
             "INSERT INTO Authors (author_id, name, bio) VALUES (%s, '%s', '%s')",
             author_id, escaped_name, escaped_bio);

    if (mysql_query(conn, query)) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Failed to insert author: %s", mysql_error(conn));
        showMessage(NULL, GTK_MESSAGE_ERROR, error_msg);
    } else {
        showMessage(NULL, GTK_MESSAGE_INFO, "Author inserted successfully.");
    }
}

void createInsertAuthorWindow(void) {
    GtkWidget *author_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(author_window), "Insert New Author");
    gtk_window_set_default_size(GTK_WINDOW(author_window), 400, 300);

    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(author_window), grid);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

    AuthorEntryWidgets *widgets = g_new(AuthorEntryWidgets, 1);
    widgets->author_id = gtk_entry_new();
    widgets->name = gtk_entry_new();
    widgets->bio = gtk_entry_new();

    const char *labels[] = {"Author ID:", "Name:", "Bio:"};
    GtkWidget *entry_widgets[] = {widgets->author_id, widgets->name, widgets->bio};

    for (int i = 0; i < 3; i++) {
        GtkWidget *label = gtk_label_new(labels[i]);
        gtk_grid_attach(GTK_GRID(grid), label, 0, i, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entry_widgets[i], 1, i, 1, 1);
    }

    GtkWidget *insert_button = gtk_button_new_with_label("Insert Author");
    gtk_grid_attach(GTK_GRID(grid), insert_button, 1, 3, 1, 1);
    g_signal_connect(insert_button, "clicked", G_CALLBACK(insertAuthorGUI), widgets);
    
    g_signal_connect_swapped(author_window, "destroy", G_CALLBACK(g_free), widgets);
    gtk_widget_show_all(author_window);
}

void viewAuthorsGUI(GtkWidget *widget, gpointer parent_window) {
    GtkTreeIter iter;
    GtkWidget *view_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(view_window), "View Authors");
    gtk_window_set_default_size(GTK_WINDOW(view_window), 600, 400);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(view_window), scrolled_window);

    GtkListStore *store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree);

    const char *titles[] = {"Author ID", "Name", "Bio"};
    for (int i = 0; i < 3; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(titles[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    }

    char query[] = "SELECT author_id, name, bio FROM Authors";
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *res = mysql_store_result(conn);
        if (res) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, 0, row[0], 1, row[1], 2, row[2], -1);
            }
            mysql_free_result(res);
        }
    }

    gtk_widget_show_all(view_window);
}

void showPublicRegistrationWindow(void) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Public Registration");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);

    const char *labels[] = {"Username:", "Password:", "Name:", "Email:", 
                           "Phone:", "Address:"};
    GtkWidget *entries[6];
    
    for (int i = 0; i < 6; i++) {
        GtkWidget *label = gtk_label_new(labels[i]);
        entries[i] = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(grid), label, 0, i, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entries[i], 1, i, 1, 1);
    }

    // Make password entry hidden
    gtk_entry_set_visibility(GTK_ENTRY(entries[1]), FALSE);

    GtkWidget *submit = gtk_button_new_with_label("Register");
    gtk_grid_attach(GTK_GRID(grid), submit, 0, 6, 2, 1);

    g_signal_connect(submit, "clicked", G_CALLBACK(registerPublicUser), entries);
    gtk_widget_show_all(window);
}

void createStaffRegistrationWindow(void) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Staff Registration");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);

    const char *labels[] = {"Username:", "Password:", "Name:", "Email:", 
                           "Phone:", "Address:", "Role:"};
    GtkWidget *entries[7];
    
    for (int i = 0; i < 7; i++) {
        GtkWidget *label = gtk_label_new(labels[i]);
        entries[i] = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(grid), label, 0, i, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), entries[i], 1, i, 1, 1);
    }

    // Make password entry hidden
    gtk_entry_set_visibility(GTK_ENTRY(entries[1]), FALSE);

    GtkWidget *submit = gtk_button_new_with_label("Register");
    gtk_grid_attach(GTK_GRID(grid), submit, 0, 7, 2, 1);

    // Connect the submit button to the callback function
    g_signal_connect(submit, "clicked", G_CALLBACK(registerStaffUser), entries);
    
    // Connect the window's destroy signal to free the entries array
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_widget_destroy), NULL);
    
    gtk_widget_show_all(window);
}

static void registerPublicUser(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const char *username = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char *password = gtk_entry_get_text(GTK_ENTRY(entries[1]));
    const char *name = gtk_entry_get_text(GTK_ENTRY(entries[2]));
    const char *email = gtk_entry_get_text(GTK_ENTRY(entries[3]));
    const char *phone = gtk_entry_get_text(GTK_ENTRY(entries[4]));
    const char *address = gtk_entry_get_text(GTK_ENTRY(entries[5]));

    // Basic validation
    if (!isNonEmptyString(username, LIB_NAME_LEN) || !isNonEmptyString(password, PASSWORD_LEN) ||
        !isNonEmptyString(name, LIB_NAME_LEN) || !isNonEmptyString(email, EMAIL_LEN) ||
        !isNonEmptyString(phone, PHONE_LEN) || !isNonEmptyString(address, ADDRESS_LEN)) {
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_MESSAGE_ERROR, 
                   "All fields must be filled out.");
        return;
    }

    int member_id;
    if (registerMember(username, password, name, email, phone, address, &member_id)) {
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_MESSAGE_INFO,
                   "Registration successful! You can now login.");
        
        // Set the current user info
        current_user_id = member_id;
        strncpy(current_user_role, "Member", 19);
        current_user_role[19] = '\0';
        
        // Close the registration window
        GtkWidget *reg_window = gtk_widget_get_toplevel(widget);
        gtk_widget_destroy(reg_window);
        
        // Show the dashboard
        createDashboardWindow();
    } else {
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_MESSAGE_ERROR,
                   "Registration failed. Username may already exist or other DB error.");
    }
}

static void registerStaffUser(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const char *username = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char *password = gtk_entry_get_text(GTK_ENTRY(entries[1]));
    const char *name = gtk_entry_get_text(GTK_ENTRY(entries[2]));
    const char *email = gtk_entry_get_text(GTK_ENTRY(entries[3]));
    const char *phone = gtk_entry_get_text(GTK_ENTRY(entries[4]));
    const char *address = gtk_entry_get_text(GTK_ENTRY(entries[5]));
    const char *role = gtk_entry_get_text(GTK_ENTRY(entries[6]));

    // Basic validation
    if (!isNonEmptyString(username, LIB_NAME_LEN) || !isNonEmptyString(password, PASSWORD_LEN) ||
        !isNonEmptyString(name, LIB_NAME_LEN) || !isNonEmptyString(email, EMAIL_LEN) ||
        !isNonEmptyString(phone, PHONE_LEN) || !isNonEmptyString(address, ADDRESS_LEN) ||
        !isNonEmptyString(role, ROLE_LEN)) {
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_MESSAGE_ERROR, 
                   "All fields must be filled out.");
        return;
    }

    if (strcmp(role, "Staff") != 0 && strcmp(role, "Admin") != 0) {
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_MESSAGE_ERROR,
                   "Role must be either 'Staff' or 'Admin'.");
        return;
    }

    int staff_id;
    if (registerStaff(username, password, name, email, phone, address, role, &staff_id)) {
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_MESSAGE_INFO, 
                   "Staff registration successful!");
        
        // Set the current user info
        current_user_id = staff_id;
        strncpy(current_user_role, role, 19);
        current_user_role[19] = '\0';
        
        // Close the registration window
        GtkWidget *reg_window = gtk_widget_get_toplevel(widget);
        gtk_widget_destroy(reg_window);
        
        // Show the dashboard
        createDashboardWindow();
    } else {
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_MESSAGE_ERROR,
                   "Registration failed. Username may already exist or other DB error.");
    }
}

void runGuiLogin(int argc, char *argv[]) {
    gtk_init(&argc, &argv);

    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), "Library Management Login");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 300, 200);
    gtk_window_set_position(GTK_WINDOW(main_window), GTK_WIN_POS_CENTER);
    g_signal_connect(main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(main_window), grid);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

    LoginWidgets *widgets = g_new(LoginWidgets, 1);
    widgets->username = gtk_entry_new();
    widgets->password = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(widgets->password), FALSE);

    GtkWidget *label_username = gtk_label_new("Username:");
    GtkWidget *label_password = gtk_label_new("Password:");
    GtkWidget *login_button = gtk_button_new_with_label("Login");
    GtkWidget *register_button = gtk_button_new_with_label("Register as Member");
    GtkWidget *staff_register_button = gtk_button_new_with_label("Register as Staff");

    gtk_grid_attach(GTK_GRID(grid), label_username, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), widgets->username, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), label_password, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), widgets->password, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), login_button, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), register_button, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), staff_register_button, 1, 4, 1, 1);

    g_signal_connect(login_button, "clicked", G_CALLBACK(on_gui_login_clicked), widgets);
    g_signal_connect(register_button, "clicked", G_CALLBACK(createPublicRegistrationWindow), NULL);
    g_signal_connect(staff_register_button, "clicked", G_CALLBACK(createStaffRegistrationWindow), NULL);
    g_signal_connect_swapped(main_window, "destroy", G_CALLBACK(g_free), widgets);

    gtk_widget_show_all(main_window);
    gtk_main();
}


int main(int argc, char *argv[]) {
    connectDB();
    runGuiLogin(argc, argv);
    mysql_close(conn);
    return 0;
}

// Added GUI function for borrowing books
void createBorrowBookWindow(void) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Borrow Book");
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 150);

    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

    GtkWidget *label_book_id = gtk_label_new("Book ID:");
    GtkWidget *entry_book_id = gtk_entry_new();
    GtkWidget *borrow_button = gtk_button_new_with_label("Borrow Book");

    gtk_grid_attach(GTK_GRID(grid), label_book_id, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry_book_id, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), borrow_button, 1, 1, 1, 1);

    // Connect the borrow button to the callback function
    g_signal_connect(borrow_button, "clicked", G_CALLBACK(on_borrow_button_clicked), entry_book_id);
    
    gtk_widget_show_all(window);
}

// Callback function for the Borrow Book button
void on_borrow_button_clicked(GtkWidget *widget, gpointer data) {
    GtkWidget *entry_book_id = (GtkWidget *)data;
    const char *book_id_str = gtk_entry_get_text(GTK_ENTRY(entry_book_id));
    int book_id;

    if (!isValidInteger(book_id_str)) {
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_MESSAGE_ERROR, "Book ID must be a positive integer.");
        return;
    }
    book_id = atoi(book_id_str);

    // Check if book exists and has copies available
    char check_query[MAX_QUERY_LEN];
    snprintf(check_query, sizeof(check_query),
             "SELECT copies_available FROM Books WHERE book_id = %d", book_id);

    if (mysql_query(conn, check_query)) {
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_MESSAGE_ERROR, "Error checking book availability.");
        return;
    }
    MYSQL_RES *res = mysql_store_result(conn);
    if (!res || mysql_num_rows(res) == 0) {
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_MESSAGE_ERROR, "Book not found.");
        if (res) mysql_free_result(res);
        return;
    }
    MYSQL_ROW row = mysql_fetch_row(res);
    int copies_available = atoi(row[0]);
    mysql_free_result(res);

    if (copies_available <= 0) {
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_MESSAGE_INFO, "No copies of this book are currently available.");
        return;
    }

    // Record the borrowing
    char borrow_query[MAX_QUERY_LEN];
    // Get current date
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char borrow_date[DATE_LEN];
    snprintf(borrow_date, sizeof(borrow_date), "%d-%02d-%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

    // Use current_user_id which is set during login for members
    snprintf(borrow_query, sizeof(borrow_query),
             "INSERT INTO Borrowings (book_id, member_id, borrow_date) VALUES (%d, %d, '%s')",
             book_id, current_user_id, borrow_date);

    if (mysql_query(conn, borrow_query)) {
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_MESSAGE_ERROR, "Error recording borrowing.");
        return;
    }

    // Decrease available copies in Books table
    char update_query[MAX_QUERY_LEN];
    snprintf(update_query, sizeof(update_query),
             "UPDATE Books SET copies_available = copies_available - 1 WHERE book_id = %d", book_id);

    if (mysql_query(conn, update_query)) {
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_MESSAGE_ERROR, "Error updating book copies.");
        // Consider rolling back the insert if the update fails
    } else {
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_MESSAGE_INFO, "Book borrowed successfully!");
        // Optionally close the window after successful borrowing
        // gtk_widget_destroy(gtk_widget_get_toplevel(widget));
    }
}

// Implemented viewFinesGUI
void viewFinesGUI(GtkWidget *widget, gpointer parent_window) {
    GtkTreeIter iter;
    GtkWidget *view_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(view_window), "View Fines");
    gtk_container_set_border_width(GTK_CONTAINER(view_window), 10);
    gtk_window_set_default_size(GTK_WINDOW(view_window), 600, 400);

    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(view_window), scrolled_window);

    // Assuming Fines table has columns: fine_id, borrowing_id, fine_amount, fine_date, status
    GtkListStore *store = gtk_list_store_new(5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    GtkWidget *tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree);

    const char *titles[] = {"Fine ID", "Borrowing ID", "Amount", "Date", "Status"};
    for (int i = 0; i < 5; i++) {
        GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
        GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes(titles[i], renderer, "text", i, NULL);
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
    }

    char query[] = "SELECT fine_id, borrowing_id, fine_amount, fine_date, status FROM Fines";
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *res = mysql_store_result(conn);
        if (res) {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res))) {
                gtk_list_store_append(store, &iter);
                gtk_list_store_set(store, &iter, 0, row[0], 1, row[1], 2, row[2], 3, row[3], 4, row[4], -1);
            }
            mysql_free_result(res);
        }
    }

    gtk_widget_show_all(view_window);
}

// Function to connect to the database
void connectDB(void) {
    conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init failed.\n");
        return;
    }

    // Replace with your database details
    // Example: mysql_real_connect(conn, "localhost", "user", "password", "database", 0, NULL, 0);
    if (mysql_real_connect(conn, "localhost", "root", "kali", "library_managementdb", 3306, NULL, 0) == NULL) {
        fprintf(stderr, "Failed to connect to database: %s\n", mysql_error(conn));
        mysql_close(conn);
        conn = NULL; // Set conn to NULL to indicate connection failure
        return;
    }

    printf("Database connection successful.\n");
}

void on_gui_login_clicked(GtkWidget *widget, gpointer data) {
    LoginWidgets *widgets = (LoginWidgets *)data;
    const char *username = gtk_entry_get_text(GTK_ENTRY(widgets->username));
    const char *password = gtk_entry_get_text(GTK_ENTRY(widgets->password));

    if (!isNonEmptyString(username, LIB_NAME_LEN) || !isNonEmptyString(password, PASSWORD_LEN)) {
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_MESSAGE_ERROR, "Username and password cannot be empty.");
        return;
    }

    int user_id;
    char user_role[20];

    if (authenticateUser(username, password, &user_id, user_role)) {
        current_user_id = user_id;
        strncpy(current_user_role, user_role, 19);
        current_user_role[19] = '\0';
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_MESSAGE_INFO, "Login successful!");
       createDashboardWindow();
       printf("DEBUG: Role = %s",current_user_role);
        // gtk_widget_destroy(gtk_widget_get_toplevel(widget)); // Close the login window
    } else {
        showMessage(GTK_WINDOW(gtk_widget_get_toplevel(widget)), GTK_MESSAGE_ERROR, "Invalid username or password.");
    }
}


void createDashboardWindow(void) {
    GtkWidget *dashboard = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(dashboard), "Library Management Dashboard");
    gtk_window_set_default_size(GTK_WINDOW(dashboard), 800, 600);
    gtk_window_set_position(GTK_WINDOW(dashboard), GTK_WIN_POS_CENTER);
    g_signal_connect(dashboard, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(dashboard), grid);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

    // Create a label to show current user info
    char user_info[100];
    snprintf(user_info, sizeof(user_info), "Logged in as: %s (Role: %s)", 
             current_user_role, current_user_role);
    GtkWidget *user_label = gtk_label_new(user_info);
    gtk_grid_attach(GTK_GRID(grid), user_label, 0, 0, 2, 1);

    // Create buttons based on user role
    if (strcmp(current_user_role, "Member") == 0) {
        // Member buttons
        GtkWidget *view_books_btn = gtk_button_new_with_label("View Books");
        GtkWidget *borrow_book_btn = gtk_button_new_with_label("Borrow Book");
        
        gtk_grid_attach(GTK_GRID(grid), view_books_btn, 0, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), borrow_book_btn, 1, 1, 1, 1);

        g_signal_connect(view_books_btn, "clicked", G_CALLBACK(viewBooksGUI), dashboard);
        g_signal_connect(borrow_book_btn, "clicked", G_CALLBACK(createBorrowBookWindow), NULL);
    } else if (strcmp(current_user_role, "Staff") == 0 || strcmp(current_user_role, "Admin") == 0) {
        // Staff/Admin buttons
        GtkWidget *insert_book_btn = gtk_button_new_with_label("Insert Book");
        GtkWidget *view_books_btn = gtk_button_new_with_label("View Books");
        GtkWidget *insert_author_btn = gtk_button_new_with_label("Insert Author");
        GtkWidget *view_authors_btn = gtk_button_new_with_label("View Authors");
        GtkWidget *insert_publisher_btn = gtk_button_new_with_label("Insert Publisher");
        GtkWidget *view_publishers_btn = gtk_button_new_with_label("View Publishers");
        GtkWidget *insert_member_btn = gtk_button_new_with_label("Insert Member");
        GtkWidget *view_members_btn = gtk_button_new_with_label("View Members");
        GtkWidget *insert_staff_btn = gtk_button_new_with_label("Insert Staff");
        GtkWidget *view_staff_btn = gtk_button_new_with_label("View Staff");
        GtkWidget *insert_borrowing_btn = gtk_button_new_with_label("Insert Borrowing");
        GtkWidget *view_borrowings_btn = gtk_button_new_with_label("View Borrowings");
        GtkWidget *insert_fine_btn = gtk_button_new_with_label("Insert Fine");
        GtkWidget *view_fines_btn = gtk_button_new_with_label("View Fines");

        // Attach buttons to grid
        gtk_grid_attach(GTK_GRID(grid), insert_book_btn, 0, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), view_books_btn, 1, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), insert_author_btn, 0, 2, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), view_authors_btn, 1, 2, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), insert_publisher_btn, 0, 3, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), view_publishers_btn, 1, 3, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), insert_member_btn, 0, 4, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), view_members_btn, 1, 4, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), insert_staff_btn, 0, 5, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), view_staff_btn, 1, 5, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), insert_borrowing_btn, 0, 6, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), view_borrowings_btn, 1, 6, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), insert_fine_btn, 0, 7, 1, 1);
        gtk_grid_attach(GTK_GRID(grid), view_fines_btn, 1, 7, 1, 1);

        // Connect signals
        g_signal_connect(insert_book_btn, "clicked", G_CALLBACK(createInsertBookWindow), NULL);
        g_signal_connect(view_books_btn, "clicked", G_CALLBACK(viewBooksGUI), dashboard);
        g_signal_connect(insert_author_btn, "clicked", G_CALLBACK(createInsertAuthorWindow), NULL);
        g_signal_connect(view_authors_btn, "clicked", G_CALLBACK(viewAuthorsGUI), dashboard);
        g_signal_connect(insert_publisher_btn, "clicked", G_CALLBACK(createInsertPublisherWindow), NULL);
        g_signal_connect(view_publishers_btn, "clicked", G_CALLBACK(viewPublishersGUI), dashboard);
        g_signal_connect(insert_member_btn, "clicked", G_CALLBACK(createInsertMemberWindow), NULL);
        g_signal_connect(view_members_btn, "clicked", G_CALLBACK(viewMembersGUI), dashboard);
        g_signal_connect(insert_staff_btn, "clicked", G_CALLBACK(createInsertStaffWindow), NULL);
        g_signal_connect(view_staff_btn, "clicked", G_CALLBACK(viewStaffGUI), dashboard);
        g_signal_connect(insert_borrowing_btn, "clicked", G_CALLBACK(createInsertBorrowingWindow), NULL);
        g_signal_connect(view_borrowings_btn, "clicked", G_CALLBACK(viewBorrowingsGUI), dashboard);
        g_signal_connect(insert_fine_btn, "clicked", G_CALLBACK(createInsertFineWindow), NULL);
        g_signal_connect(view_fines_btn, "clicked", G_CALLBACK(viewFinesGUI), dashboard);
    }

    gtk_widget_show_all(dashboard);
    void createDashboardWindow(void) {
    if (!current_user_role) {
        fprintf(stderr, "Error: current_user_role is NULL\n");
        return;
    }
}
}