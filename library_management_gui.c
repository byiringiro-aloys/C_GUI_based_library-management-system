#include <gtk/gtk.h>
#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <sys/stat.h> // For mkdir function
#include <fcntl.h> // For open, close, fcntl, flock
#include <sys/file.h> // For flock
#include <unistd.h> // For read

// MySQL connection parameters
#define HOST "localhost"
#define USER "root"
#define PASS "" // Update if you have a password
#define DB "library_managementdb"

// User roles and permissions
typedef enum {
    ROLE_ADMIN = 0,     // System administrator
    ROLE_LIBRARIAN = 1, // Full control over all data
    ROLE_MEMBER = 2     // Limited to viewing and borrowing books
} UserRole;

typedef struct {
    int user_id;
    char username[50];
    char password[50];
    UserRole role;
    int member_id; // If user is member, link to member record
} User;

// Global variables
MYSQL *conn;
User current_user;

// Function declarations
void login_callback(GtkWidget *widget, gpointer data);
void handle_database_error(MYSQL *conn, const char *operation);
void handle_memory_error(void);
void show_authors_view(GtkWidget *widget, gpointer data);
void show_books_view(GtkWidget *widget, gpointer data);
void show_members_view(GtkWidget *widget, gpointer data);
void show_borrowings_view(GtkWidget *widget, gpointer data);
void show_fines_view(GtkWidget *widget, gpointer data);
void show_users_view(GtkWidget *widget, gpointer data);
void show_system_settings(GtkWidget *widget, gpointer data);
void show_system_logs(GtkWidget *widget, gpointer data);
void backup_database(GtkWidget *widget, gpointer data);
GtkWidget* create_stat_box(const char* title, int value);
GtkWidget* create_dashboard(void);
GtkWidget* create_table_view(const char* title, const char* query, int num_columns, const char** column_names);

// Show unauthorized access message
void show_unauthorized_message(GtkWidget *parent) {
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_ERROR,
                                              GTK_BUTTONS_OK,
                                              "Unauthorized Access: You don't have permission to perform this action.");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Authentication window
void show_login_window(void) {
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Library Management System - Login");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 150);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    // Username
    GtkWidget *username_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *username_label = gtk_label_new("Username:");
    GtkWidget *username_entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(username_hbox), username_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(username_hbox), username_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), username_hbox, FALSE, FALSE, 0);

    // Password
    GtkWidget *password_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget *password_label = gtk_label_new("Password:");
    GtkWidget *password_entry = gtk_entry_new();
    gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
    gtk_box_pack_start(GTK_BOX(password_hbox), password_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(password_hbox), password_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), password_hbox, FALSE, FALSE, 0);

    // Login button
    GtkWidget *login_button = gtk_button_new_with_label("Login");
    g_signal_connect(login_button, "clicked", G_CALLBACK(login_callback), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), login_button, FALSE, FALSE, 0);

    gtk_widget_show_all(window);
}

// Login callback
void login_callback(GtkWidget *widget, gpointer data) {
    GtkWidget *window = gtk_widget_get_toplevel(widget);
    GtkWidget *username_entry = gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(window))))));
    GtkWidget *password_entry = gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(window))))))));

    const char *username = gtk_entry_get_text(GTK_ENTRY(username_entry));
    const char *password = gtk_entry_get_text(GTK_ENTRY(password_entry));

    if (authenticate_user(username, password)) {
        gtk_widget_destroy(window);
        show_main_window();
    } else {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR,
                                                  GTK_BUTTONS_OK,
                                                  "Invalid username or password!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

// Authenticate user with prepared statement
int authenticate_user(const char *username, const char *password) {
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (!stmt) {
        handle_database_error(conn, "Initializing prepared statement");
        return 0;
    }

    const char *query = "SELECT user_id, username, password, role, member_id FROM Users WHERE username = ? AND password = ?";
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        handle_database_error(conn, "Preparing statement");
        mysql_stmt_close(stmt);
        return 0;
    }

    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (void *)username;
    bind[0].buffer_length = strlen(username);
    bind[0].is_null = 0;

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (void *)password;
    bind[1].buffer_length = strlen(password);
    bind[1].is_null = 0;

    if (mysql_stmt_bind_param(stmt, bind)) {
        handle_database_error(conn, "Binding parameters");
        mysql_stmt_close(stmt);
        return 0;
    }

    if (mysql_stmt_execute(stmt)) {
        handle_database_error(conn, "Executing statement");
        mysql_stmt_close(stmt);
        return 0;
    }

    MYSQL_BIND result_bind[5];
    memset(result_bind, 0, sizeof(result_bind));

    int user_id;
    char username_buf[50];
    char password_buf[50];
    int role;
    int member_id;
    my_bool is_null[5] = {0};

    result_bind[0].buffer_type = MYSQL_TYPE_LONG;
    result_bind[0].buffer = &user_id;
    result_bind[0].is_null = &is_null[0];

    result_bind[1].buffer_type = MYSQL_TYPE_STRING;
    result_bind[1].buffer = username_buf;
    result_bind[1].buffer_length = sizeof(username_buf);
    result_bind[1].is_null = &is_null[1];

    result_bind[2].buffer_type = MYSQL_TYPE_STRING;
    result_bind[2].buffer = password_buf;
    result_bind[2].buffer_length = sizeof(password_buf);
    result_bind[2].is_null = &is_null[2];

    result_bind[3].buffer_type = MYSQL_TYPE_LONG;
    result_bind[3].buffer = &role;
    result_bind[3].is_null = &is_null[3];

    result_bind[4].buffer_type = MYSQL_TYPE_LONG;
    result_bind[4].buffer = &member_id;
    result_bind[4].is_null = &is_null[4];

    if (mysql_stmt_bind_result(stmt, result_bind)) {
        handle_database_error(conn, "Binding results");
        mysql_stmt_close(stmt);
        return 0;
    }

    if (mysql_stmt_fetch(stmt)) {
        mysql_stmt_close(stmt);
        return 0;
    }

    current_user.user_id = user_id;
    strncpy(current_user.username, username_buf, sizeof(current_user.username) - 1);
    strncpy(current_user.password, password_buf, sizeof(current_user.password) - 1);
    current_user.role = role;
    current_user.member_id = member_id;

    mysql_stmt_close(stmt);
    return 1;
}

// Check if user has required role
int check_permission(UserRole required_role) {
    return current_user.role <= required_role;
}

// Logout user
void logout_user(void) {
    memset(&current_user, 0, sizeof(User));
    show_login_window();
}

// Log transaction to file
void log_transaction(const char *action, const char *details) {
    FILE *log_file = fopen("library_transactions.log", "a");
    if (!log_file) {
        g_print("Error opening log file!\n");
        return;
    }
    time_t now = time(NULL);
    fprintf(log_file, "[%s] %s: %s\n", ctime(&now), action, details);
    fclose(log_file);
}

// Handle MySQL errors with proper cleanup
void finish_with_error(GtkWidget *parent) {
    const char *error = mysql_error(conn);
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(parent),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_ERROR,
                                              GTK_BUTTONS_OK,
                                              "MySQL Error: %s", error);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    // Clean up database connection
    if (conn) {
        mysql_close(conn);
        conn = NULL;
    }
    
    // Clean up GTK
    gtk_main_quit();
}

// Generic list function for displaying results in a text view
void list_records(GtkWidget *widget, gpointer data) {
    struct { GtkTextBuffer *buffer; const char *query; const char *action; } *params = data;
    if (mysql_query(conn, params->query)) {
        finish_with_error(gtk_widget_get_toplevel(widget));
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        finish_with_error(gtk_widget_get_toplevel(widget));
    }

    gtk_text_buffer_set_text(params->buffer, "", -1);
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(params->buffer, &iter);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        GString *line = g_string_new("");
        if (!line) {
            handle_memory_error();
            return;
        }
        for (int i = 0; i < mysql_num_fields(result); i++) {
            g_string_append_printf(line, "%s: %s | ", mysql_fetch_field_direct(result, i)->name, row[i] ? row[i] : "NULL");
        }
        g_string_append(line, "\n");
        gtk_text_buffer_insert(params->buffer, &iter, line->str, -1);
        g_string_free(line, TRUE);
    }
    mysql_free_result(result);
    log_transaction(params->action, "Listed records");
}

// Add Author with prepared statement
void add_author_cb(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const char *name = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char *bio = gtk_entry_get_text(GTK_ENTRY(entries[1]));

    // Validate input
    if (!name || !bio) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR,
                                                  GTK_BUTTONS_OK,
                                                  "Please fill in all required fields!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (!stmt) {
        handle_database_error(conn, "Initializing prepared statement");
        return;
    }

    const char *query = "INSERT INTO Authors (name, bio) VALUES (?, ?)";
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        handle_database_error(conn, "Preparing statement");
        mysql_stmt_close(stmt);
        return;
    }

    MYSQL_BIND bind[2];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (void *)name;
    bind[0].buffer_length = strlen(name);
    bind[0].is_null = 0;

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (void *)bio;
    bind[1].buffer_length = strlen(bio);
    bind[1].is_null = 0;

    if (mysql_stmt_bind_param(stmt, bind)) {
        handle_database_error(conn, "Binding parameters");
        mysql_stmt_close(stmt);
        return;
    }

    if (mysql_stmt_execute(stmt)) {
        handle_database_error(conn, "Executing statement");
        mysql_stmt_close(stmt);
        return;
    }

    mysql_stmt_close(stmt);

    log_transaction("Add Author", name);
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_OK,
                                              "Author '%s' added successfully!", name);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Add Publisher
void add_publisher_cb(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const char *name = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char *address = gtk_entry_get_text(GTK_ENTRY(entries[1]));
    const char *contact_info = gtk_entry_get_text(GTK_ENTRY(entries[2]));

    char query[512];
    snprintf(query, sizeof(query),
             "INSERT INTO Publishers (name, address, contact_info) VALUES ('%s', '%s', '%s')",
             name, address, contact_info);

    if (mysql_query(conn, query)) {
        finish_with_error(gtk_widget_get_toplevel(widget));
    }

    log_transaction("Add Publisher", name);
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                              GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                              "Publisher '%s' added successfully!", name);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Add Book
void add_book_cb(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const char *title = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char *author_id = gtk_entry_get_text(GTK_ENTRY(entries[1]));
    const char *publisher_id = gtk_entry_get_text(GTK_ENTRY(entries[2]));
    const char *isbn = gtk_entry_get_text(GTK_ENTRY(entries[3]));
    const char *genre = gtk_entry_get_text(GTK_ENTRY(entries[4]));
    const char *year = gtk_entry_get_text(GTK_ENTRY(entries[5]));
    const char *copies = gtk_entry_get_text(GTK_ENTRY(entries[6]));
    const char *shelf = gtk_entry_get_text(GTK_ENTRY(entries[7]));

    // Validate foreign keys
    char query[256];
    snprintf(query, sizeof(query), "SELECT author_id FROM Authors WHERE author_id = %s", author_id);
    if (mysql_query(conn, query) || !mysql_store_result(conn)->row_count) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                                  GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                  "Invalid author_id!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }
    snprintf(query, sizeof(query), "SELECT publisher_id FROM Publishers WHERE publisher_id = %s", publisher_id);
    if (mysql_query(conn, query) || !mysql_store_result(conn)->row_count) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                                  GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                  "Invalid publisher_id!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    snprintf(query, sizeof(query),
             "INSERT INTO Books (title, author_id, publisher_id, isbn, genre, year_published, copies_available, shelf_location) "
             "VALUES ('%s', %s, %s, '%s', '%s', %s, %s, '%s')",
             title, author_id, publisher_id, isbn, genre, year, copies, shelf);

    if (mysql_query(conn, query)) {
        finish_with_error(gtk_widget_get_toplevel(widget));
    }

    log_transaction("Add Book", title);
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                              GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                              "Book '%s' added successfully!", title);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Update Book
void update_book_cb(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const char *book_id = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char *copies = gtk_entry_get_text(GTK_ENTRY(entries[1]));

    char query[256];
    snprintf(query, sizeof(query), "UPDATE Books SET copies_available = %s WHERE book_id = %s", copies, book_id);

    if (mysql_query(conn, query)) {
        finish_with_error(gtk_widget_get_toplevel(widget));
    }

    char log_details[100];
    snprintf(log_details, sizeof(log_details), "Book ID %s updated copies to %s", book_id, copies);
    log_transaction("Update Book", log_details);
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                              GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                              "Book ID %s updated successfully!", book_id);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Delete Book
void delete_book_cb(GtkWidget *widget, gpointer data) {
    const char *book_id = gtk_entry_get_text(GTK_ENTRY(data));
    char query[256];
    snprintf(query, sizeof(query), "DELETE FROM Books WHERE book_id = %s", book_id);

    if (mysql_query(conn, query)) {
        finish_with_error(gtk_widget_get_toplevel(widget));
    }

    char log_details[100];
    snprintf(log_details, sizeof(log_details), "Book ID %s deleted", book_id);
    log_transaction("Delete Book", log_details);
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                              GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                              "Book ID %s deleted successfully!", book_id);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Add Member
void add_member_cb(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const char *name = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char *address = gtk_entry_get_text(GTK_ENTRY(entries[1]));
    const char *phone = gtk_entry_get_text(GTK_ENTRY(entries[2]));
    const char *email = gtk_entry_get_text(GTK_ENTRY(entries[3]));
    const char *date_joined = gtk_entry_get_text(GTK_ENTRY(entries[4]));
    const char *status = gtk_entry_get_text(GTK_ENTRY(entries[5]));

    char query[512];
    snprintf(query, sizeof(query),
             "INSERT INTO Members (name, address, phone, email, date_joined, membership_status) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s')",
             name, address, phone, email, date_joined, status);

    if (mysql_query(conn, query)) {
        finish_with_error(gtk_widget_get_toplevel(widget));
    }

    log_transaction("Add Member", name);
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                              GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                              "Member '%s' added successfully!", name);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Borrow Book
void borrow_book_cb(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const char *book_id = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char *member_id = gtk_entry_get_text(GTK_ENTRY(entries[1]));
    const char *staff_id = gtk_entry_get_text(GTK_ENTRY(entries[2]));
    const char *borrow_date = gtk_entry_get_text(GTK_ENTRY(entries[3]));
    const char *due_date = gtk_entry_get_text(GTK_ENTRY(entries[4]));

    // Validate book availability
    char query[256];
    snprintf(query, sizeof(query), "SELECT copies_available FROM Books WHERE book_id = %s", book_id);
    if (mysql_query(conn, query)) {
        finish_with_error(gtk_widget_get_toplevel(widget));
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result || mysql_num_rows(result) == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                                  GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                  "Book not found!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        mysql_free_result(result);
        return;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    int copies = atoi(row[0]);
    mysql_free_result(result);

    if (copies <= 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                                  GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                  "No copies available!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    // Insert borrowing
    snprintf(query, sizeof(query),
             "INSERT INTO Borrowings (book_id, member_id, borrow_date, due_date, staff_id) "
             "VALUES (%s, %s, '%s', '%s', %s)",
             book_id, member_id, borrow_date, due_date, staff_id);

    if (mysql_query(conn, query)) {
        finish_with_error(gtk_widget_get_toplevel(widget));
    }

    // Update book copies
    snprintf(query, sizeof(query), "UPDATE Books SET copies_available = copies_available - 1 WHERE book_id = %s", book_id);
    if (mysql_query(conn, query)) {
        finish_with_error(gtk_widget_get_toplevel(widget));
    }

    char log_details[100];
    snprintf(log_details, sizeof(log_details), "Book ID %s borrowed by Member ID %s", book_id, member_id);
    log_transaction("Borrow Book", log_details);
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                              GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                              "Book borrowed successfully!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Return Book
void return_book_cb(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const char *borrowing_id = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char *return_date = gtk_entry_get_text(GTK_ENTRY(entries[1]));

    // Update borrowing
    char query[256];
    snprintf(query, sizeof(query),
             "UPDATE Borrowings SET return_date = '%s' WHERE borrowing_id = %s",
             return_date, borrowing_id);

    if (mysql_query(conn, query)) {
        finish_with_error(gtk_widget_get_toplevel(widget));
    }

    // Get book_id
    snprintf(query, sizeof(query), "SELECT book_id FROM Borrowings WHERE borrowing_id = %s", borrowing_id);
    if (mysql_query(conn, query)) {
        finish_with_error(gtk_widget_get_toplevel(widget));
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result || mysql_num_rows(result) == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                                  GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                  "Borrowing not found!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        mysql_free_result(result);
        return;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    const char *book_id = row[0];
    mysql_free_result(result);

    // Update book copies
    snprintf(query, sizeof(query), "UPDATE Books SET copies_available = copies_available + 1 WHERE book_id = %s", book_id);
    if (mysql_query(conn, query)) {
        finish_with_error(gtk_widget_get_toplevel(widget));
    }

    char log_details[100];
    snprintf(log_details, sizeof(log_details), "Book ID %s returned for Borrowing ID %s", book_id, borrowing_id);
    log_transaction("Return Book", log_details);
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                              GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                              "Book returned successfully!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Add Fine
void add_fine_cb(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const char *borrowing_id = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char *amount = gtk_entry_get_text(GTK_ENTRY(entries[1]));

    char query[256];
    snprintf(query, sizeof(query),
             "INSERT INTO Fines (borrowing_id, amount, paid) VALUES (%s, %s, FALSE)",
             borrowing_id, amount);

    if (mysql_query(conn, query)) {
        finish_with_error(gtk_widget_get_toplevel(widget));
    }

    char log_details[100];
    snprintf(log_details, sizeof(log_details), "Fine of %s added for Borrowing ID %s", amount, borrowing_id);
    log_transaction("Add Fine", log_details);
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                              GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                              "Fine added successfully!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Pay Fine
void pay_fine_cb(GtkWidget *widget, gpointer data) {
    GtkWidget **entries = (GtkWidget **)data;
    const char *fine_id = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char *date_paid = gtk_entry_get_text(GTK_ENTRY(entries[1]));

    char query[256];
    snprintf(query, sizeof(query),
             "UPDATE Fines SET paid = TRUE, date_paid = '%s' WHERE fine_id = %s",
             date_paid, fine_id);

    if (mysql_query(conn, query)) {
        finish_with_error(gtk_widget_get_toplevel(widget));
    }

    char log_details[100];
    snprintf(log_details, sizeof(log_details), "Fine ID %s paid", fine_id);
    log_transaction("Pay Fine", log_details);
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                              GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
                                              "Fine paid successfully!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Handle memory allocation errors
void handle_memory_error(void) {
    GtkWidget* dialog = gtk_message_dialog_new(NULL,
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_ERROR,
                                              GTK_BUTTONS_OK,
                                              "Memory allocation failed!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    exit(1);
}

// Create form window with proper memory management
GtkWidget* create_form_window(const char* title, const char* labels[], int num_fields, GCallback callback, gpointer data) {
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (!window) {
        handle_memory_error();
        return NULL;
    }
    
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 50 * (num_fields + 2));
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_widget_destroy), NULL);

    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    if (!vbox) {
        handle_memory_error();
        return NULL;
    }
    gtk_container_add(GTK_CONTAINER(window), vbox);

    GtkWidget** entries = g_new(GtkWidget*, num_fields);
    if (!entries) {
        handle_memory_error();
        return NULL;
    }

    for (int i = 0; i < num_fields; i++) {
        GtkWidget* hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        if (!hbox) {
            g_free(entries);
            handle_memory_error();
            return NULL;
        }
        
        GtkWidget* label = gtk_label_new(labels[i]);
        if (!label) {
            g_free(entries);
            handle_memory_error();
            return NULL;
        }
        
        entries[i] = gtk_entry_new();
        if (!entries[i]) {
            g_free(entries);
            handle_memory_error();
            return NULL;
        }
        
        gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(hbox), entries[i], TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    }

    // Button box for Submit and Back buttons
    GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    if (!button_box) {
        g_free(entries);
        handle_memory_error();
        return NULL;
    }
    gtk_box_set_homogeneous(GTK_BOX(button_box), TRUE);

    // Submit button
    GtkWidget* submit_button = gtk_button_new_with_label("Submit");
    if (!submit_button) {
        g_free(entries);
        handle_memory_error();
        return NULL;
    }
    g_signal_connect(submit_button, "clicked", callback, entries);
    gtk_box_pack_start(GTK_BOX(button_box), submit_button, TRUE, TRUE, 0);

    // Back button
    GtkWidget* back_button = gtk_button_new_with_label("Back");
    if (!back_button) {
        g_free(entries);
        handle_memory_error();
        return NULL;
    }
    g_signal_connect(back_button, "clicked", G_CALLBACK(gtk_widget_destroy), window);
    gtk_box_pack_start(GTK_BOX(button_box), back_button, TRUE, TRUE, 0);

    gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);

    gtk_widget_show_all(window);
    return window;
}

// Dashboard statistics structure
typedef struct {
    int total_books;
    int available_books;
    int total_members;
    int active_borrowings;
    int pending_fines;
} DashboardStats;

// Create interactive table view with proper memory management
GtkWidget* create_table_view(const char* title, const char* query, int num_columns, const char** column_names) {
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    if (!window) {
        handle_memory_error();
        return NULL;
    }
    
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    if (!vbox) {
        gtk_widget_destroy(window);
        handle_memory_error();
        return NULL;
    }
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    // Search box
    GtkWidget* search_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    if (!search_box) {
        gtk_widget_destroy(window);
        handle_memory_error();
        return NULL;
    }
    
    GtkWidget* search_entry = gtk_entry_new();
    if (!search_entry) {
        gtk_widget_destroy(window);
        handle_memory_error();
        return NULL;
    }
    gtk_entry_set_placeholder_text(GTK_ENTRY(search_entry), "Search...");
    
    GtkWidget* search_button = gtk_button_new_with_label("Search");
    if (!search_button) {
        gtk_widget_destroy(window);
        handle_memory_error();
        return NULL;
    }
    
    gtk_box_pack_start(GTK_BOX(search_box), search_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(search_box), search_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), search_box, FALSE, FALSE, 0);
    
    // Create tree view
    GtkWidget* scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    if (!scrolled_window) {
        gtk_widget_destroy(window);
        handle_memory_error();
        return NULL;
    }
    
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                 GTK_POLICY_AUTOMATIC,
                                 GTK_POLICY_AUTOMATIC);
    
    // Create store with correct number of columns
    GType* types = g_new(GType, num_columns);
    if (!types) {
        gtk_widget_destroy(window);
        handle_memory_error();
        return NULL;
    }
    
    for (int i = 0; i < num_columns; i++) {
        types[i] = G_TYPE_STRING;
    }
    
    GtkListStore* store = gtk_list_store_newv(num_columns, types);
    g_free(types);
    
    if (!store) {
        gtk_widget_destroy(window);
        handle_memory_error();
        return NULL;
    }
    
    GtkWidget* tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
    g_object_unref(store); // Unref the store as tree_view now owns it
    
    if (!tree_view) {
        gtk_widget_destroy(window);
        handle_memory_error();
        return NULL;
    }
    
    // Add columns
    for (int i = 0; i < num_columns; i++) {
        GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
        if (!renderer) {
            gtk_widget_destroy(window);
            handle_memory_error();
            return NULL;
        }
        
        GtkTreeViewColumn* column = gtk_tree_view_column_new_with_attributes(
            column_names[i], renderer, "text", i, NULL);
        if (!column) {
            gtk_widget_destroy(window);
            handle_memory_error();
            return NULL;
        }
        
        gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), column);
    }
    
    gtk_container_add(GTK_CONTAINER(scrolled_window), tree_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
    
    // Action buttons
    GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    if (!button_box) {
        gtk_widget_destroy(window);
        handle_memory_error();
        return NULL;
    }
    
    GtkWidget* add_button = gtk_button_new_with_label("Add New");
    GtkWidget* edit_button = gtk_button_new_with_label("Edit");
    GtkWidget* delete_button = gtk_button_new_with_label("Delete");
    GtkWidget* refresh_button = gtk_button_new_with_label("Refresh");
    
    if (!add_button || !edit_button || !delete_button || !refresh_button) {
        gtk_widget_destroy(window);
        handle_memory_error();
        return NULL;
    }
    
    gtk_box_pack_start(GTK_BOX(button_box), add_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), edit_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), delete_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(button_box), refresh_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);
    
    // Load data using prepared statement
    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (!stmt) {
        gtk_widget_destroy(window);
        handle_database_error(conn, "Initializing prepared statement");
        return NULL;
    }
    
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        gtk_widget_destroy(window);
        handle_database_error(conn, "Preparing statement");
        mysql_stmt_close(stmt);
        return NULL;
    }
    
    if (mysql_stmt_execute(stmt)) {
        gtk_widget_destroy(window);
        handle_database_error(conn, "Executing statement");
        mysql_stmt_close(stmt);
        return NULL;
    }
    
    MYSQL_RES* result = mysql_stmt_result_metadata(stmt);
    if (!result) {
        gtk_widget_destroy(window);
        handle_database_error(conn, "Getting result metadata");
        mysql_stmt_close(stmt);
        return NULL;
    }
    
    MYSQL_BIND* bind = g_new(MYSQL_BIND, num_columns);
    if (!bind) {
        gtk_widget_destroy(window);
        mysql_free_result(result);
        mysql_stmt_close(stmt);
        handle_memory_error();
        return NULL;
    }
    
    char** data = g_new(char*, num_columns);
    if (!data) {
        gtk_widget_destroy(window);
        g_free(bind);
        mysql_free_result(result);
        mysql_stmt_close(stmt);
        handle_memory_error();
        return NULL;
    }
    
    unsigned long* lengths = g_new(unsigned long, num_columns);
    if (!lengths) {
        gtk_widget_destroy(window);
        g_free(data);
        g_free(bind);
        mysql_free_result(result);
        mysql_stmt_close(stmt);
        handle_memory_error();
        return NULL;
    }
    
    my_bool* is_null = g_new(my_bool, num_columns);
    if (!is_null) {
        gtk_widget_destroy(window);
        g_free(lengths);
        g_free(data);
        g_free(bind);
        mysql_free_result(result);
        mysql_stmt_close(stmt);
        handle_memory_error();
        return NULL;
    }
    
    for (int i = 0; i < num_columns; i++) {
        data[i] = g_new(char, 256); // Maximum field length
        if (!data[i]) {
            for (int j = 0; j < i; j++) {
                g_free(data[j]);
            }
            gtk_widget_destroy(window);
            g_free(is_null);
            g_free(lengths);
            g_free(data);
            g_free(bind);
            mysql_free_result(result);
            mysql_stmt_close(stmt);
            handle_memory_error();
            return NULL;
        }
        
        bind[i].buffer_type = MYSQL_TYPE_STRING;
        bind[i].buffer = data[i];
        bind[i].buffer_length = 256;
        bind[i].length = &lengths[i];
        bind[i].is_null = &is_null[i];
    }
    
    if (mysql_stmt_bind_result(stmt, bind)) {
        for (int i = 0; i < num_columns; i++) {
            g_free(data[i]);
        }
        gtk_widget_destroy(window);
        g_free(is_null);
        g_free(lengths);
        g_free(data);
        g_free(bind);
        mysql_free_result(result);
        mysql_stmt_close(stmt);
        handle_database_error(conn, "Binding results");
        return NULL;
    }
    
    while (mysql_stmt_fetch(stmt) == 0) {
        GtkTreeIter iter;
        gtk_list_store_append(store, &iter);
        for (int i = 0; i < num_columns; i++) {
            if (!is_null[i]) {
                gtk_list_store_set(store, &iter, i, data[i], -1);
            } else {
                gtk_list_store_set(store, &iter, i, "NULL", -1);
            }
        }
    }
    
    // Clean up
    for (int i = 0; i < num_columns; i++) {
        g_free(data[i]);
    }
    g_free(is_null);
    g_free(lengths);
    g_free(data);
    g_free(bind);
    mysql_free_result(result);
    mysql_stmt_close(stmt);
    
    gtk_widget_show_all(window);
    return window;
}

// Create dashboard view
GtkWidget* create_dashboard(void) {
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Library Dashboard");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    // Statistics grid
    GtkWidget* stats_grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(stats_grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(stats_grid), 10);
    
    // Get statistics
    DashboardStats stats;
    char query[256];
    
    // Total books
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM Books");
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES* result = mysql_store_result(conn);
        if (result) {
            MYSQL_ROW row = mysql_fetch_row(result);
            stats.total_books = atoi(row[0]);
            mysql_free_result(result);
        }
    }
    
    // Available books
    snprintf(query, sizeof(query), "SELECT SUM(copies_available) FROM Books");
    if (mysql_query(conn, query) == 0) {
        MYSQL_RES* result = mysql_store_result(conn);
        if (result) {
            MYSQL_ROW row = mysql_fetch_row(result);
            stats.available_books = atoi(row[0]);
            mysql_free_result(result);
        }
    }
    
    // Create stat boxes
    GtkWidget* total_books_box = create_stat_box("Total Books", stats.total_books);
    GtkWidget* available_books_box = create_stat_box("Available Books", stats.available_books);
    GtkWidget* total_members_box = create_stat_box("Total Members", stats.total_members);
    GtkWidget* active_borrowings_box = create_stat_box("Active Borrowings", stats.active_borrowings);
    GtkWidget* pending_fines_box = create_stat_box("Pending Fines", stats.pending_fines);
    
    gtk_grid_attach(GTK_GRID(stats_grid), total_books_box, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(stats_grid), available_books_box, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(stats_grid), total_members_box, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(stats_grid), active_borrowings_box, 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(stats_grid), pending_fines_box, 1, 1, 1, 1);
    
    gtk_box_pack_start(GTK_BOX(vbox), stats_grid, FALSE, FALSE, 0);
    
    // Quick action buttons
    GtkWidget* action_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget* authors_btn = gtk_button_new_with_label("Manage Authors");
    GtkWidget* books_btn = gtk_button_new_with_label("Manage Books");
    GtkWidget* members_btn = gtk_button_new_with_label("Manage Members");
    GtkWidget* borrowings_btn = gtk_button_new_with_label("Manage Borrowings");
    GtkWidget* fines_btn = gtk_button_new_with_label("Manage Fines");
    
    gtk_box_pack_start(GTK_BOX(action_box), authors_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(action_box), books_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(action_box), members_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(action_box), borrowings_btn, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(action_box), fines_btn, TRUE, TRUE, 0);
    
    gtk_box_pack_start(GTK_BOX(vbox), action_box, FALSE, FALSE, 0);
    
    // Connect signals
    g_signal_connect(authors_btn, "clicked", G_CALLBACK(show_authors_view), NULL);
    g_signal_connect(books_btn, "clicked", G_CALLBACK(show_books_view), NULL);
    g_signal_connect(members_btn, "clicked", G_CALLBACK(show_members_view), NULL);
    g_signal_connect(borrowings_btn, "clicked", G_CALLBACK(show_borrowings_view), NULL);
    g_signal_connect(fines_btn, "clicked", G_CALLBACK(show_fines_view), NULL);
    
    gtk_widget_show_all(window);
    return window;
}

// Create stat box
GtkWidget* create_stat_box(const char* title, int value) {
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_widget_set_size_request(box, 150, 100);
    
    GtkWidget* title_label = gtk_label_new(title);
    gtk_label_set_line_wrap(GTK_LABEL(title_label), TRUE);
    gtk_label_set_justify(GTK_LABEL(title_label), GTK_JUSTIFY_CENTER);
    
    char value_str[32];
    snprintf(value_str, sizeof(value_str), "%d", value);
    GtkWidget* value_label = gtk_label_new(value_str);
    gtk_label_set_line_wrap(GTK_LABEL(value_label), TRUE);
    gtk_label_set_justify(GTK_LABEL(value_label), GTK_JUSTIFY_CENTER);
    
    gtk_box_pack_start(GTK_BOX(box), title_label, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(box), value_label, TRUE, TRUE, 0);
    
    return box;
}

// View callbacks
void show_authors_view(GtkWidget* widget, gpointer data) {
    const char* column_names[] = {"ID", "Name", "Bio"};
    create_table_view("Authors", "SELECT author_id, name, bio FROM Authors", 3, column_names);
}

void show_books_view(GtkWidget* widget, gpointer data) {
    const char* column_names[] = {"ID", "Title", "Author", "Publisher", "ISBN", "Genre", "Year", "Copies", "Shelf"};
    create_table_view("Books", 
        "SELECT b.book_id, b.title, a.name, p.name, b.isbn, b.genre, b.year_published, b.copies_available, b.shelf_location "
        "FROM Books b "
        "JOIN Authors a ON b.author_id = a.author_id "
        "JOIN Publishers p ON b.publisher_id = p.publisher_id", 
        9, column_names);
}

void show_members_view(GtkWidget* widget, gpointer data) {
    const char* column_names[] = {"ID", "Name", "Address", "Phone", "Email", "Date Joined", "Status"};
    create_table_view("Members", 
        "SELECT member_id, name, address, phone, email, date_joined, membership_status FROM Members", 
        7, column_names);
}

void show_borrowings_view(GtkWidget* widget, gpointer data) {
    const char* column_names[] = {"ID", "Book", "Member", "Borrow Date", "Due Date", "Return Date", "Status"};
    create_table_view("Borrowings", 
        "SELECT b.borrowing_id, bk.title, m.name, b.borrow_date, b.due_date, b.return_date, "
        "CASE WHEN b.return_date IS NULL THEN 'Active' ELSE 'Returned' END as status "
        "FROM Borrowings b "
        "JOIN Books bk ON b.book_id = bk.book_id "
        "JOIN Members m ON b.member_id = m.member_id", 
        7, column_names);
}

void show_fines_view(GtkWidget* widget, gpointer data) {
    const char* column_names[] = {"ID", "Borrowing", "Amount", "Paid", "Date Paid"};
    create_table_view("Fines", 
        "SELECT f.fine_id, b.borrowing_id, f.amount, f.paid, f.date_paid FROM Fines f "
        "JOIN Borrowings b ON f.borrowing_id = b.borrowing_id", 
        5, column_names);
}

// User management functions
void show_users_view(GtkWidget* widget, gpointer data) {
    const char* column_names[] = {"ID", "Username", "Role", "Member ID", "Last Login"};
    create_table_view("Users", 
        "SELECT user_id, username, "
        "CASE role WHEN 0 THEN 'Admin' WHEN 1 THEN 'Librarian' WHEN 2 THEN 'Member' END as role, "
        "member_id, last_login FROM Users", 
        5, column_names);
}

// Add user with prepared statement
void add_user_cb(GtkWidget* widget, gpointer data) {
    GtkWidget** entries = (GtkWidget **)data;
    const char* username = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char* password = gtk_entry_get_text(GTK_ENTRY(entries[1]));
    const char* role = gtk_entry_get_text(GTK_ENTRY(entries[2]));
    const char* member_id = gtk_entry_get_text(GTK_ENTRY(entries[3]));

    // Validate input
    if (!username || !password || !role) {
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR,
                                                  GTK_BUTTONS_OK,
                                                  "Please fill in all required fields!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    MYSQL_STMT *stmt = mysql_stmt_init(conn);
    if (!stmt) {
        handle_database_error(conn, "Initializing prepared statement");
        return;
    }

    const char *query = "INSERT INTO Users (username, password, role, member_id) VALUES (?, ?, ?, ?)";
    if (mysql_stmt_prepare(stmt, query, strlen(query))) {
        handle_database_error(conn, "Preparing statement");
        mysql_stmt_close(stmt);
        return;
    }

    MYSQL_BIND bind[4];
    memset(bind, 0, sizeof(bind));

    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (void *)username;
    bind[0].buffer_length = strlen(username);
    bind[0].is_null = 0;

    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (void *)password;
    bind[1].buffer_length = strlen(password);
    bind[1].is_null = 0;

    int role_val = atoi(role);
    bind[2].buffer_type = MYSQL_TYPE_LONG;
    bind[2].buffer = &role_val;
    bind[2].is_null = 0;

    int member_id_val = member_id ? atoi(member_id) : 0;
    bind[3].buffer_type = MYSQL_TYPE_LONG;
    bind[3].buffer = &member_id_val;
    bind[3].is_null = member_id ? 0 : 1;

    if (mysql_stmt_bind_param(stmt, bind)) {
        handle_database_error(conn, "Binding parameters");
        mysql_stmt_close(stmt);
        return;
    }

    if (mysql_stmt_execute(stmt)) {
        handle_database_error(conn, "Executing statement");
        mysql_stmt_close(stmt);
        return;
    }

    mysql_stmt_close(stmt);

    log_transaction("Add User", username);
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_OK,
                                              "User '%s' added successfully!", username);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void edit_user_cb(GtkWidget* widget, gpointer data) {
    GtkWidget** entries = (GtkWidget **)data;
    const char* user_id = gtk_entry_get_text(GTK_ENTRY(entries[0]));
    const char* username = gtk_entry_get_text(GTK_ENTRY(entries[1]));
    const char* password = gtk_entry_get_text(GTK_ENTRY(entries[2]));
    const char* role = gtk_entry_get_text(GTK_ENTRY(entries[3]));
    const char* member_id = gtk_entry_get_text(GTK_ENTRY(entries[4]));

    // Validate input
    if (!user_id || !username || !role) {
        GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR,
                                                  GTK_BUTTONS_OK,
                                                  "Please fill in all required fields!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    // Update user
    char query[512];
    if (password && *password) {
        snprintf(query, sizeof(query),
                 "UPDATE Users SET username = '%s', password = '%s', role = %s, member_id = %s "
                 "WHERE user_id = %s",
                 username, password, role, member_id ? member_id : "NULL", user_id);
    } else {
        snprintf(query, sizeof(query),
                 "UPDATE Users SET username = '%s', role = %s, member_id = %s "
                 "WHERE user_id = %s",
                 username, role, member_id ? member_id : "NULL", user_id);
    }

    if (mysql_query(conn, query)) {
        handle_database_error(conn, "Updating user");
        return;
    }

    log_transaction("Edit User", username);
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_OK,
                                              "User '%s' updated successfully!", username);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

void delete_user_cb(GtkWidget* widget, gpointer data) {
    const char* user_id = gtk_entry_get_text(GTK_ENTRY(data));

    // Confirm deletion
    GtkWidget* dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_QUESTION,
                                              GTK_BUTTONS_YES_NO,
                                              "Are you sure you want to delete this user?");
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (response == GTK_RESPONSE_YES) {
        char query[256];
        snprintf(query, sizeof(query), "DELETE FROM Users WHERE user_id = %s", user_id);
        
        if (mysql_query(conn, query)) {
            handle_database_error(conn, "Deleting user");
            return;
        }
        
        log_transaction("Delete User", user_id);
        dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(widget)),
                                       GTK_DIALOG_MODAL,
                                       GTK_MESSAGE_INFO,
                                       GTK_BUTTONS_OK,
                                       "User deleted successfully!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

// System management functions
void show_system_settings(GtkWidget* widget, gpointer data) {
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "System Settings");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    // Database settings
    GtkWidget* db_frame = gtk_frame_new("Database Settings");
    GtkWidget* db_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(db_frame), db_box);
    
    GtkWidget* host_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(host_entry), "Database Host");
    gtk_entry_set_text(GTK_ENTRY(host_entry), HOST);
    
    GtkWidget* user_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(user_entry), "Database User");
    gtk_entry_set_text(GTK_ENTRY(user_entry), USER);
    
    GtkWidget* pass_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(pass_entry), "Database Password");
    gtk_entry_set_text(GTK_ENTRY(pass_entry), PASS);
    gtk_entry_set_visibility(GTK_ENTRY(pass_entry), FALSE);
    
    GtkWidget* db_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(db_entry), "Database Name");
    gtk_entry_set_text(GTK_ENTRY(db_entry), DB);
    
    gtk_box_pack_start(GTK_BOX(db_box), host_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(db_box), user_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(db_box), pass_entry, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(db_box), db_entry, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(vbox), db_frame, FALSE, FALSE, 0);
    
    // System settings
    GtkWidget* sys_frame = gtk_frame_new("System Settings");
    GtkWidget* sys_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(sys_frame), sys_box);
    
    GtkWidget* log_level_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(log_level_combo), "DEBUG");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(log_level_combo), "INFO");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(log_level_combo), "WARNING");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(log_level_combo), "ERROR");
    gtk_combo_box_set_active(GTK_COMBO_BOX(log_level_combo), 1);
    
    GtkWidget* auto_backup_check = gtk_check_button_new_with_label("Enable Automatic Backup");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(auto_backup_check), TRUE);
    
    GtkWidget* backup_interval_spin = gtk_spin_button_new_with_range(1, 24, 1);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(backup_interval_spin), 6);
    
    gtk_box_pack_start(GTK_BOX(sys_box), gtk_label_new("Log Level:"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sys_box), log_level_combo, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sys_box), auto_backup_check, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sys_box), gtk_label_new("Backup Interval (hours):"), FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(sys_box), backup_interval_spin, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(vbox), sys_frame, FALSE, FALSE, 0);
    
    // Save button
    GtkWidget* save_button = gtk_button_new_with_label("Save Settings");
    g_signal_connect(save_button, "clicked", G_CALLBACK(save_system_settings), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), save_button, FALSE, FALSE, 0);
    
    gtk_widget_show_all(window);
}

void show_system_logs(GtkWidget* widget, gpointer data) {
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "System Logs");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    // Filter options
    GtkWidget* filter_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    GtkWidget* date_from = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(date_from), "Date From (YYYY-MM-DD)");
    GtkWidget* date_to = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(date_to), "Date To (YYYY-MM-DD)");
    GtkWidget* level_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(level_combo), "All Levels");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(level_combo), "DEBUG");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(level_combo), "INFO");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(level_combo), "WARNING");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(level_combo), "ERROR");
    GtkWidget* filter_button = gtk_button_new_with_label("Filter");
    
    gtk_box_pack_start(GTK_BOX(filter_box), date_from, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), date_to, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), level_combo, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(filter_box), filter_button, FALSE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(vbox), filter_box, FALSE, FALSE, 0);
    
    // Log view
    GtkWidget* scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    GtkWidget* text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
    gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
    
    // Load logs
    FILE* log_file = fopen("library_transactions.log", "r");
    if (log_file) {
        char line[1024];
        GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        while (fgets(line, sizeof(line), log_file)) {
            gtk_text_buffer_insert_at_cursor(buffer, line, -1);
        }
        fclose(log_file);
    }
    
    gtk_widget_show_all(window);
}

// Backup database with proper error handling
void backup_database(GtkWidget *widget, gpointer data) {
    // Create backup directory if it doesn't exist
    const char *backup_dir = "backups";
    if (mkdir(backup_dir, 0755) != 0 && errno != EEXIST) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR,
                                                  GTK_BUTTONS_OK,
                                                  "Failed to create backup directory: %s",
                                                  strerror(errno));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return;
    }

    // Generate backup filename with timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char backup_file[256];
    snprintf(backup_file, sizeof(backup_file), "%s/backup_%04d%02d%02d_%02d%02d%02d.sql",
             backup_dir, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);

    // Escape database parameters to prevent command injection
    char *escaped_host = g_shell_quote(HOST);
    char *escaped_user = g_shell_quote(USER);
    char *escaped_pass = g_shell_quote(PASS);
    char *escaped_db = g_shell_quote(DB);
    char *escaped_file = g_shell_quote(backup_file);

    // Construct backup command
    char *command = g_strdup_printf("mysqldump -h %s -u %s -p%s %s > %s",
                                   escaped_host, escaped_user, escaped_pass,
                                   escaped_db, escaped_file);

    // Execute backup command
    int result = system(command);

    // Free allocated memory
    g_free(escaped_host);
    g_free(escaped_user);
    g_free(escaped_pass);
    g_free(escaped_db);
    g_free(escaped_file);
    g_free(command);

    // Check backup result
    if (result == 0) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_INFO,
                                                  GTK_BUTTONS_OK,
                                                  "Database backup created successfully!\nBackup file: %s",
                                                  backup_file);
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    } else {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR,
                                                  GTK_BUTTONS_OK,
                                                  "Failed to create database backup!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

// System statistics helper functions
int get_total_users(void) {
    char query[256];
    snprintf(query, sizeof(query), "SELECT COUNT(*) FROM Users");
    
    if (mysql_query(conn, query)) {
        handle_database_error(conn, "Getting total users");
        return 0;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        handle_database_error(conn, "Storing result");
        return 0;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    int count = row ? atoi(row[0]) : 0;
    mysql_free_result(result);
    return count;
}

int get_active_users(void) {
    char query[256];
    snprintf(query, sizeof(query), 
             "SELECT COUNT(DISTINCT user_id) FROM Users WHERE last_login >= DATE_SUB(NOW(), INTERVAL 24 HOUR)");
    
    if (mysql_query(conn, query)) {
        handle_database_error(conn, "Getting active users");
        return 0;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        handle_database_error(conn, "Storing result");
        return 0;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    int count = row ? atoi(row[0]) : 0;
    mysql_free_result(result);
    return count;
}

int get_total_operations(void) {
    // Count total operations from log file
    FILE* log_file = fopen("library_transactions.log", "r");
    if (!log_file) return 0;
    
    int count = 0;
    char line[1024];
    while (fgets(line, sizeof(line), log_file)) {
        count++;
    }
    fclose(log_file);
    return count;
}

// Get system uptime with proper file locking
int get_system_uptime(void) {
    const char *uptime_file = "system_start_time";
    int uptime = 0;
    
    // Open file with exclusive lock
    int fd = open(uptime_file, O_RDONLY);
    if (fd == -1) {
        return 0;
    }
    
    // Try to get exclusive lock
    struct flock fl = {
        .l_type = F_RDLCK,
        .l_whence = SEEK_SET,
        .l_start = 0,
        .l_len = 0
    };
    
    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        close(fd);
        return 0;
    }
    
    // Read start time
    time_t start_time;
    if (read(fd, &start_time, sizeof(start_time)) == sizeof(start_time)) {
        time_t now = time(NULL);
        uptime = (int)(now - start_time) / 3600; // Return uptime in hours
    }
    
    // Release lock
    fl.l_type = F_UNLCK;
    fcntl(fd, F_SETLK, &fl);
    close(fd);
    
    return uptime;
}

// Handle file operations with proper error checking
int safe_file_operation(const char *filename, const char *mode, const char *operation) {
    FILE *file = fopen(filename, mode);
    if (!file) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR,
                                                  GTK_BUTTONS_OK,
                                                  "Failed to %s file '%s': %s",
                                                  operation, filename, strerror(errno));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return 0;
    }
    fclose(file);
    return 1;
}

// Save system settings with proper error handling
void save_system_settings(GtkWidget *widget, gpointer data) {
    const char *config_file = "library_config.ini";
    
    // Check if we can write to the config file
    if (!safe_file_operation(config_file, "w", "write to")) {
        return;
    }

    // Get database settings
    GtkWidget *window = gtk_widget_get_toplevel(widget);
    GtkWidget *vbox = gtk_bin_get_child(GTK_BIN(window));
    GtkWidget *db_frame = gtk_bin_get_child(GTK_BIN(vbox));
    GtkWidget *db_vbox = gtk_bin_get_child(GTK_BIN(db_frame));
    
    GtkWidget *host_entry = gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(db_vbox))));
    GtkWidget *user_entry = gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(db_vbox))))));
    GtkWidget *pass_entry = gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(db_vbox))))))));
    GtkWidget *db_entry = gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(db_vbox)))))))));

    const char *host = gtk_entry_get_text(GTK_ENTRY(host_entry));
    const char *user = gtk_entry_get_text(GTK_ENTRY(user_entry));
    const char *pass = gtk_entry_get_text(GTK_ENTRY(pass_entry));
    const char *db = gtk_entry_get_text(GTK_ENTRY(db_entry));

    // Get system settings
    GtkWidget *sys_frame = gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(vbox))));
    GtkWidget *sys_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(sys_frame), sys_box);
    
    GtkWidget *log_level_combo = gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(sys_box))));
    GtkWidget *auto_backup_check = gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(sys_box))))));
    GtkWidget *backup_interval_spin = gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(gtk_bin_get_child(GTK_BIN(sys_box))))))));

    const char *log_level = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(log_level_combo));
    gboolean auto_backup = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(auto_backup_check));
    int backup_interval = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(backup_interval_spin));

    // Open config file for writing
    FILE *file = fopen(config_file, "w");
    if (!file) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR,
                                                  GTK_BUTTONS_OK,
                                                  "Failed to open config file for writing!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        g_free((gpointer)log_level);
        return;
    }

    // Write settings to file
    fprintf(file, "[Database]\n");
    fprintf(file, "host = %s\n", host);
    fprintf(file, "user = %s\n", user);
    fprintf(file, "password = %s\n", pass);
    fprintf(file, "database = %s\n\n", db);

    fprintf(file, "[System]\n");
    fprintf(file, "log_level = %s\n", log_level);
    fprintf(file, "auto_backup = %s\n", auto_backup ? "true" : "false");
    fprintf(file, "backup_interval = %d\n", backup_interval);

    fclose(file);
    g_free((gpointer)log_level);

    // Show success message
    GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_INFO,
                                              GTK_BUTTONS_OK,
                                              "Settings saved successfully!");
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Modify create_main_window to include admin features
void create_main_window() {
    if (current_user.role == ROLE_ADMIN) {
        // Create admin dashboard
        GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "Library Management System - Admin");
        gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

        GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_container_add(GTK_CONTAINER(window), vbox);

        // User info and logout
        GtkWidget* user_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        char user_info[100];
        snprintf(user_info, sizeof(user_info), "Logged in as: %s (Admin)", current_user.username);
        GtkWidget* user_label = gtk_label_new(user_info);
        GtkWidget* logout_button = gtk_button_new_with_label("Logout");
        g_signal_connect(logout_button, "clicked", G_CALLBACK(logout_user), NULL);
        gtk_box_pack_start(GTK_BOX(user_hbox), user_label, TRUE, TRUE, 0);
        gtk_box_pack_end(GTK_BOX(user_hbox), logout_button, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), user_hbox, FALSE, FALSE, 0);

        // Admin action buttons
        GtkWidget* button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        GtkWidget* users_btn = gtk_button_new_with_label("Manage Users");
        GtkWidget* system_btn = gtk_button_new_with_label("System Settings");
        GtkWidget* logs_btn = gtk_button_new_with_label("View Logs");
        GtkWidget* backup_btn = gtk_button_new_with_label("Backup Database");

        g_signal_connect(users_btn, "clicked", G_CALLBACK(show_users_view), NULL);
        g_signal_connect(system_btn, "clicked", G_CALLBACK(show_system_settings), NULL);
        g_signal_connect(logs_btn, "clicked", G_CALLBACK(show_system_logs), NULL);
        g_signal_connect(backup_btn, "clicked", G_CALLBACK(backup_database), NULL);

        gtk_box_pack_start(GTK_BOX(button_box), users_btn, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(button_box), system_btn, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(button_box), logs_btn, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(button_box), backup_btn, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), button_box, FALSE, FALSE, 0);

        // System statistics
        GtkWidget* stats_grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(stats_grid), 10);
        gtk_grid_set_column_spacing(GTK_GRID(stats_grid), 10);

        // Add system statistics
        GtkWidget* total_users_box = create_stat_box("Total Users", get_total_users());
        GtkWidget* active_users_box = create_stat_box("Active Users", get_active_users());
        GtkWidget* total_operations_box = create_stat_box("Total Operations", get_total_operations());
        GtkWidget* system_uptime_box = create_stat_box("System Uptime", get_system_uptime());

        gtk_grid_attach(GTK_GRID(stats_grid), total_users_box, 0, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(stats_grid), active_users_box, 1, 0, 1, 1);
        gtk_grid_attach(GTK_GRID(stats_grid), total_operations_box, 0, 1, 1, 1);
        gtk_grid_attach(GTK_GRID(stats_grid), system_uptime_box, 1, 1, 1, 1);

        gtk_box_pack_start(GTK_BOX(vbox), stats_grid, FALSE, FALSE, 0);

        gtk_widget_show_all(window);
    } else if (current_user.role == ROLE_LIBRARIAN) {
        create_dashboard();
    } else {
        // Existing member view code
        // ... existing code ...
    }
}

// Handle database errors
void handle_database_error(MYSQL *conn, const char *operation) {
    const char *error = mysql_error(conn);
    GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                              GTK_DIALOG_MODAL,
                                              GTK_MESSAGE_ERROR,
                                              GTK_BUTTONS_OK,
                                              "Database Error during %s: %s", operation, error);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

// Initialize database connection
int init_database(void) {
    conn = mysql_init(NULL);
    if (!conn) {
        g_print("mysql_init() failed\n");
        return 0;
    }

    // Set connection timeout
    int timeout = 10;
    mysql_options(conn, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    // Enable automatic reconnection
    my_bool reconnect = 1;
    mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);

    // Connect to database
    if (!mysql_real_connect(conn, HOST, USER, PASS, DB, 0, NULL, 0)) {
        g_print("MySQL connection failed: %s\n", mysql_error(conn));
        return 0;
    }

    // Set character set
    if (mysql_set_character_set(conn, "utf8mb4")) {
        g_print("Failed to set character set: %s\n", mysql_error(conn));
        return 0;
    }

    return 1;
}

// Main function with proper initialization and cleanup
int main(int argc, char *argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);

    // Initialize database connection
    if (!init_database()) {
        GtkWidget *dialog = gtk_message_dialog_new(NULL,
                                                  GTK_DIALOG_MODAL,
                                                  GTK_MESSAGE_ERROR,
                                                  GTK_BUTTONS_OK,
                                                  "Failed to initialize database connection!");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return 1;
    }

    // Show login window
    show_login_window();

    // Start GTK main loop
    gtk_main();

    // Cleanup
    if (conn) {
        mysql_close(conn);
    }

    return 0;
}