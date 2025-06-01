#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>
#include <time.h>

// MySQL connection parameters
#define HOST "localhost"
#define USER "root"
#define PASS "" // Default XAMPP, update if different
#define DB "library_management_db"

// Function to log transactions
void log_transaction(const char *action, const char *details) {
    FILE *log_file = fopen("library_transactions.log", "a");
    if (!log_file) {
        printf("Error opening log file!\n");
        return;
    }
    time_t now = time(NULL);
    fprintf(log_file, "[%s] %s: %s\n", ctime(&now), action, details);
    fclose(log_file);
}

// Handle MySQL errors
void finish_with_error(MYSQL *conn) {
    fprintf(stderr, "MySQL Error: %s\n", mysql_error(conn));
    mysql_close(conn);
    exit(1);
}

// Add Author
void add_author(MYSQL *conn) {
    char name[255], bio[1000];
    printf("Enter author name: ");
    scanf(" %[^\n]", name);
    printf("Enter bio: ");
    scanf(" %[^\n]", bio);

    char query[2048];
    snprintf(query, sizeof(query),
             "INSERT INTO Authors (name, bio) VALUES ('%s', '%s')",
             name, bio);

    if (mysql_query(conn, query)) {
        finish_with_error(conn);
    }
    log_transaction("Add Author", name);
    printf("Author added successfully!\n");
}

// List Authors
void list_authors(MYSQL *conn) {
    if (mysql_query(conn, "SELECT author_id, name, bio FROM Authors")) {
        finish_with_error(conn);
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        finish_with_error(conn);
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        printf("ID: %s, Name: %s, Bio: %s\n", row[0], row[1], row[2] ? row[2] : "NULL");
    }
    mysql_free_result(result);
}

// Add Publisher
void add_publisher(MYSQL *conn) {
    char name[255], address[255], contact_info[255];
    printf("Enter publisher name: ");
    scanf(" %[^\n]", name);
    printf("Enter address: ");
    scanf(" %[^\n]", address);
    printf("Enter contact info: ");
    scanf(" %[^\n]", contact_info);

    char query[512];
    snprintf(query, sizeof(query),
             "INSERT INTO Publishers (name, address, contact_info) VALUES ('%s', '%s', '%s')",
             name, address, contact_info);

    if (mysql_query(conn, query)) {
        finish_with_error(conn);
    }
    log_transaction("Add Publisher", name);
    printf("Publisher added successfully!\n");
}

// List Publishers
void list_publishers(MYSQL *conn) {
    if (mysql_query(conn, "SELECT publisher_id, name, address, contact_info FROM Publishers")) {
        finish_with_error(conn);
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        finish_with_error(conn);
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        printf("ID: %s, Name: %s, Address: %s, Contact: %s\n",
               row[0], row[1], row[2] ? row[2] : "NULL", row[3] ? row[3] : "NULL");
    }
    mysql_free_result(result);
}

// Add Book
void add_book(MYSQL *conn) {
    char title[255], isbn[20], genre[50], shelf_location[50];
    int author_id, publisher_id, year_published, copies_available;
    printf("Enter title: ");
    scanf(" %[^\n]", title);
    printf("Enter author_id: ");
    scanf("%d", &author_id);
    printf("Enter publisher_id: ");
    scanf("%d", &publisher_id);
    printf("Enter ISBN: ");
    scanf("%s", isbn);
    printf("Enter genre: ");
    scanf("%s", genre);
    printf("Enter year published: ");
    scanf("%d", &year_published);
    printf("Enter copies available: ");
    scanf("%d", &copies_available);
    printf("Enter shelf location: ");
    scanf("%s", shelf_location);

    char query[512];
    snprintf(query, sizeof(query),
             "INSERT INTO Books (title, author_id, publisher_id, isbn, genre, year_published, copies_available, shelf_location) "
             "VALUES ('%s', %d, %d, '%s', '%s', %d, %d, '%s')",
             title, author_id, publisher_id, isbn, genre, year_published, copies_available, shelf_location);

    if (mysql_query(conn, query)) {
        finish_with_error(conn);
    }
    log_transaction("Add Book", title);
    printf("Book added successfully!\n");
}

// List Books
void list_books(MYSQL *conn) {
    if (mysql_query(conn, "SELECT book_id, title, author_id, publisher_id, isbn, genre, year_published, copies_available, shelf_location FROM Books")) {
        finish_with_error(conn);
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        finish_with_error(conn);
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        printf("ID: %s, Title: %s, Author ID: %s, Publisher ID: %s, ISBN: %s, Genre: %s, Year: %s, Copies: %s, Shelf: %s\n",
               row[0], row[1], row[2], row[3], row[4], row[5], row[6], row[7], row[8]);
    }
    mysql_free_result(result);
}

// Update Book
void update_book(MYSQL *conn) {
    int book_id, copies_available;
    printf("Enter book_id to update: ");
    scanf("%d", &book_id);
    printf("Enter new copies available: ");
    scanf("%d", &copies_available);

    char query[256];
    snprintf(query, sizeof(query),
             "UPDATE Books SET copies_available = %d WHERE book_id = %d",
             copies_available, book_id);

    if (mysql_query(conn, query)) {
        finish_with_error(conn);
    }
    char log_details[100];
    snprintf(log_details, sizeof(log_details), "Book ID %d updated copies to %d", book_id, copies_available);
    log_transaction("Update Book", log_details);
    printf("Book updated successfully!\n");
}

// Delete Book
void delete_book(MYSQL *conn) {
    int book_id;
    printf("Enter book_id to delete: ");
    scanf("%d", &book_id);

    char query[256];
    snprintf(query, sizeof(query), "DELETE FROM Books WHERE book_id = %d", book_id);

    if (mysql_query(conn, query)) {
        finish_with_error(conn);
    }
    char log_details[100];
    snprintf(log_details, sizeof(log_details), "Book ID %d deleted", book_id);
    log_transaction("Delete Book", log_details);
    printf("Book deleted successfully!\n");
}

// Add Member
void add_member(MYSQL *conn) {
    char name[255], address[255], phone[20], email[255], date_joined[11], membership_status[50];
    printf("Enter member name: ");
    scanf(" %[^\n]", name);
    printf("Enter address: ");
    scanf(" %[^\n]", address);
    printf("Enter phone: ");
    scanf("%s", phone);
    printf("Enter email: ");
    scanf("%s", email);
    printf("Enter date joined (YYYY-MM-DD): ");
    scanf("%s", date_joined);
    printf("Enter membership status: ");
    scanf("%s", membership_status);

    char query[512];
    snprintf(query, sizeof(query),
             "INSERT INTO Members (name, address, phone, email, date_joined, membership_status) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s')",
             name, address, phone, email, date_joined, membership_status);

    if (mysql_query(conn, query)) {
        finish_with_error(conn);
    }
    log_transaction("Add Member", name);
    printf("Member added successfully!\n");
}

// List Members
void list_members(MYSQL *conn) {
    if (mysql_query(conn, "SELECT member_id, name, address, phone, email, date_joined, membership_status FROM Members")) {
        finish_with_error(conn);
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        finish_with_error(conn);
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        printf("ID: %s, Name: %s, Address: %s, Phone: %s, Email: %s, Joined: %s, Status: %s\n",
               row[0], row[1], row[2], row[3], row[4], row[5], row[6]);
    }
    mysql_free_result(result);
}

// Add Staff
void add_staff(MYSQL *conn) {
    char name[255], role[50], email[255], phone[20];
    printf("Enter staff name: ");
    scanf(" %[^\n]", name);
    printf("Enter role: ");
    scanf("%s", role);
    printf("Enter email: ");
    scanf("%s", email);
    printf("Enter phone: ");
    scanf("%s", phone);

    char query[512];
    snprintf(query, sizeof(query),
             "INSERT INTO Staff (name, role, email, phone) VALUES ('%s', '%s', '%s', '%s')",
             name, role, email, phone);

    if (mysql_query(conn, query)) {
        finish_with_error(conn);
    }
    log_transaction("Add Staff", name);
    printf("Staff added successfully!\n");
}

// List Staff
void list_staff(MYSQL *conn) {
    if (mysql_query(conn, "SELECT staff_id, name, role, email, phone FROM Staff")) {
        finish_with_error(conn);
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        finish_with_error(conn);
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        printf("ID: %s, Name: %s, Role: %s, Email: %s, Phone: %s\n",
               row[0], row[1], row[2], row[3], row[4]);
    }
    mysql_free_result(result);
}

// Borrow Book
void borrow_book(MYSQL *conn) {
    int book_id, member_id, staff_id;
    char borrow_date[11], due_date[11];
    printf("Enter book_id: ");
    scanf("%d", &book_id);
    printf("Enter member_id: ");
    scanf("%d", &member_id);
    printf("Enter staff_id: ");
    scanf("%d", &staff_id);
    printf("Enter borrow date (YYYY-MM-DD): ");
    scanf("%s", borrow_date);
    printf("Enter due date (YYYY-MM-DD): ");
    scanf("%s", due_date);

    // Check if book is available
    char query[256];
    snprintf(query, sizeof(query), "SELECT copies_available FROM Books WHERE book_id = %d", book_id);
    if (mysql_query(conn, query)) {
        finish_with_error(conn);
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result || mysql_num_rows(result) == 0) {
        printf("Book not found!\n");
        mysql_free_result(result);
        return;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    int copies = atoi(row[0]);
    mysql_free_result(result);

    if (copies <= 0) {
        printf("No copies available!\n");
        return;
    }

    // Insert borrowing record
    snprintf(query, sizeof(query),
             "INSERT INTO Borrowings (book_id, member_id, borrow_date, due_date, staff_id) "
             "VALUES (%d, %d, '%s', '%s', %d)",
             book_id, member_id, borrow_date, due_date, staff_id);

    if (mysql_query(conn, query)) {
        finish_with_error(conn);
    }

    // Update book copies
    snprintf(query, sizeof(query), "UPDATE Books SET copies_available = copies_available - 1 WHERE book_id = %d", book_id);
    if (mysql_query(conn, query)) {
        finish_with_error(conn);
    }

    char log_details[100];
    snprintf(log_details, sizeof(log_details), "Book ID %d borrowed by Member ID %d", book_id, member_id);
    log_transaction("Borrow Book", log_details);
    printf("Book borrowed successfully!\n");
}

// Return Book
void return_book(MYSQL *conn) {
    int borrowing_id;
    char return_date[11];
    printf("Enter borrowing_id: ");
    scanf("%d", &borrowing_id);
    printf("Enter return date (YYYY-MM-DD): ");
    scanf("%s", return_date);

    // Update borrowing record
    char query[256];
    snprintf(query, sizeof(query),
             "UPDATE Borrowings SET return_date = '%s' WHERE borrowing_id = %d",
             return_date, borrowing_id);

    if (mysql_query(conn, query)) {
        finish_with_error(conn);
    }

    // Get book_id
    snprintf(query, sizeof(query), "SELECT book_id FROM Borrowings WHERE borrowing_id = %d", borrowing_id);
    if (mysql_query(conn, query)) {
        finish_with_error(conn);
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result || mysql_num_rows(result) == 0) {
        printf("Borrowing not found!\n");
        mysql_free_result(result);
        return;
    }

    MYSQL_ROW row = mysql_fetch_row(result);
    int book_id = atoi(row[0]);
    mysql_free_result(result);

    // Update book copies
    snprintf(query, sizeof(query), "UPDATE Books SET copies_available = copies_available + 1 WHERE book_id = %d", book_id);
    if (mysql_query(conn, query)) {
        finish_with_error(conn);
    }

    char log_details[100];
    snprintf(log_details, sizeof(log_details), "Book ID %d returned for Borrowing ID %d", book_id, borrowing_id);
    log_transaction("Return Book", log_details);
    printf("Book returned successfully!\n");
}

// Add Fine
void add_fine(MYSQL *conn) {
    int borrowing_id;
    float amount;
    printf("Enter borrowing_id: ");
    scanf("%d", &borrowing_id);
    printf("Enter fine amount: ");
    scanf("%f", &amount);

    char query[256];
    snprintf(query, sizeof(query),
             "INSERT INTO Fines (borrowing_id, amount, paid) VALUES (%d, %.2f, FALSE)",
             borrowing_id, amount);

    if (mysql_query(conn, query)) {
        finish_with_error(conn);
    }
    char log_details[100];
    snprintf(log_details, sizeof(log_details), "Fine of %.2f added for Borrowing ID %d", amount, borrowing_id);
    log_transaction("Add Fine", log_details);
    printf("Fine added successfully!\n");
}

// Pay Fine
void pay_fine(MYSQL *conn) {
    int fine_id;
    char date_paid[11];
    printf("Enter fine_id: ");
    scanf("%d", &fine_id);
    printf("Enter date paid (YYYY-MM-DD): ");
    scanf("%s", date_paid);

    char query[256];
    snprintf(query, sizeof(query),
             "UPDATE Fines SET paid = TRUE, date_paid = '%s' WHERE fine_id = %d",
             date_paid, fine_id);

    if (mysql_query(conn, query)) {
        finish_with_error(conn);
    }
    char log_details[100];
    snprintf(log_details, sizeof(log_details), "Fine ID %d paid", fine_id);
    log_transaction("Pay Fine", log_details);
    printf("Fine paid successfully!\n");
}

// List Fines
void list_fines(MYSQL *conn) {
    if (mysql_query(conn, "SELECT fine_id, borrowing_id, amount, paid, date_paid FROM Fines")) {
        finish_with_error(conn);
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        finish_with_error(conn);
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result))) {
        printf("ID: %s, Borrowing ID: %s, Amount: %s, Paid: %s, Date Paid: %s\n",
               row[0], row[1], row[2], row[3], row[4] ? row[4] : "NULL");
    }
    mysql_free_result(result);
}

// Main Menu
void menu(MYSQL *conn) {
    int choice;
    while (1) {
        printf("\n=== Library Management System ===\n");
        printf("1. Add Author\n2. List Authors\n3. Add Publisher\n4. List Publishers\n");
        printf("5. Add Book\n6. List Books\n7. Update Book\n8. Delete Book\n");
        printf("9. Add Member\n10. List Members\n11. Add Staff\n12. List Staff\n");
        printf("13. Borrow Book\n14. Return Book\n15. Add Fine\n16. Pay Fine\n17. List Fines\n18. Exit\n");
        printf("Enter choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: add_author(conn); break;
            case 2: list_authors(conn); break;
            case 3: add_publisher(conn); break;
            case 4: list_publishers(conn); break;
            case 5: add_book(conn); break;
            case 6: list_books(conn); break;
            case 7: update_book(conn); break;
            case 8: delete_book(conn); break;
            case 9: add_member(conn); break;
            case 10: list_members(conn); break;
            case 11: add_staff(conn); break;
            case 12: list_staff(conn); break;
            case 13: borrow_book(conn); break;
            case 14: return_book(conn); break;
            case 15: add_fine(conn); break;
            case 16: pay_fine(conn); break;
            case 17: list_fines(conn); break;
            case 18: return;
            default: printf("Invalid choice!\n");
        }
    }
}

int main() {
    MYSQL *conn = mysql_init(NULL);
    if (!conn) {
        fprintf(stderr, "mysql_init() failed\n");
        return 1;
    }

    if (!mysql_real_connect(conn, HOST, USER, PASS, DB, 0, NULL, 0)) {
        finish_with_error(conn);
    }

    menu(conn);
    mysql_close(conn);
    return 0;
}