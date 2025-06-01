
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>

// Database connection details
#define HOST "localhost"
#define USER "root"
#define PASS "" 
#define DB "crud_in_c_db"

// Function to handle MySQL connection
MYSQL* connect_db() {
    MYSQL *conn = mysql_init(NULL);
    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return NULL;
    }  
    if (mysql_real_connect(conn, HOST, USER, PASS, DB, 0, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }
    return conn;
}

// Create a new user
void create_user(MYSQL *conn, const char *name, const char *email) {
    char query[256];
    snprintf(query, sizeof(query), "INSERT INTO users (name, email) VALUES ('%s', '%s')", name, email);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "CREATE failed: %s\n", mysql_error(conn));
    } else {
        printf("User created successfully!\n");
    }
}

// Read all users
void read_users(MYSQL *conn) {
    if (mysql_query(conn, "SELECT * FROM users")) {
        fprintf(stderr, "READ failed: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    if (result == NULL) {
        fprintf(stderr, "mysql_store_result() failed: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_ROW row;
    printf("\nUsers List:\n");
    printf("ID | Name | Email\n");
    printf("-------------------------------\n");
    while ((row = mysql_fetch_row(result))) {
        printf("%s | %s | %s\n", row[0], row[1], row[2]);
    }
    mysql_free_result(result);
}

// Update a user by ID
void update_user(MYSQL *conn, int id, const char *name, const char *email) {
    char query[256];
    snprintf(query, sizeof(query), "UPDATE users SET name='%s', email='%s' WHERE id=%d", name, email, id);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "UPDATE failed: %s\n", mysql_error(conn));
    } else {
        printf("User updated successfully!\n");
    }
}

// Delete a user by ID
void delete_user(MYSQL *conn, int id) {
    char query[256];
    snprintf(query, sizeof(query), "DELETE FROM users WHERE id=%d", id);
    if (mysql_query(conn, query)) {
        fprintf(stderr, "DELETE failed: %s\n", mysql_error(conn));
    } else {
        printf("User deleted successfully!\n");
    }
}

int main(int argc, char *argv[]) {
    MYSQL *conn = connect_db();
    if (conn == NULL) {
        system("pause"); // Pause to see error
        return 1;
    }

    int choice, id;
    char name[100], email[100];

    while (1) {
        printf("\nCRUD Menu:\n");
        printf("1. Create User\n");
        printf("2. Read Users\n");
        printf("3. Update User\n");
        printf("4. Delete User\n");
        printf("5. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        getchar(); // Clear newline from input buffer

        switch (choice) {
            case 1:
                printf("Enter name: ");
                fgets(name, sizeof(name), stdin);
                name[strcspn(name, "\n")] = 0; // Remove newline
                printf("Enter email: ");
                fgets(email, sizeof(email), stdin);
                email[strcspn(email, "\n")] = 0; // Remove newline
                create_user(conn, name, email);
                break;
            case 2:
                read_users(conn);
                break;
            case 3:
                printf("Enter user ID to update: ");
                scanf("%d", &id);
                getchar(); // Clear newline
                printf("Enter new name: ");
                fgets(name, sizeof(name), stdin);
                name[strcspn(name, "\n")] = 0;
                printf("Enter new email: ");
                fgets(email, sizeof(email), stdin);
                email[strcspn(email, "\n")] = 0;
                update_user(conn, id, name, email);
                break;
            case 4:
                printf("Enter user ID to delete: ");
                scanf("%d", &id);
                delete_user(conn, id);
                break;
            case 5:
                mysql_close(conn);
                printf("Exiting...\n");
                system("pause"); // Pause before exit
                return 0;
            default:
                printf("Invalid choice!\n");
        }
    }
	system("pause");
    return 0;
}