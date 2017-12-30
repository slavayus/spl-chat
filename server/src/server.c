#include "server.h"


int main() {
    MYSQL *conn = connect_to_database();
    print_database_info();
    create_tables(conn);

    printf("\n");


    int listener_d = open_listener_socket();

    bind_to_port(listener_d);

    set_limit_listeners(listener_d);

    pthread_t *streams = malloc(sizeof(pthread_t) * limit_listeners);

    node_t *list = malloc(sizeof(node_t));
    list->next = NULL;
    client_work_arguments *args = malloc(sizeof(args));

    int i = 0;
    while (1) {
        int connect_d = connect_client(listener_d);
        add_node(list, connect_d);
        args->connect_d_arg = connect_d;
        args->node = list;
        args->conn = conn;
        print_list(list);
        if (pthread_create(&streams[i], NULL, client_work, args) == -1) {
            error("Can't create steam");
        }
        if (i >= 10) {
            break;
        }
    }
    mysql_close(conn);
}

void *client_work(void *args) {
    client_work_arguments *actual_args = args;
    int actual_connect_d = actual_args->connect_d_arg;
    if (send_to_client_all_messages(actual_connect_d, actual_args->conn) != -1) {
        while (1) {
            char *buf = malloc(buf_len);
            size_t len = read_in(actual_connect_d, buf, buf_len);
            send_all_clients(buf, actual_args->node, actual_connect_d);
            if (strncmp("exit", buf, len - 1) == 0) {
                close(actual_connect_d);
                remove_node(actual_args->node, actual_connect_d);
                printf("STOP:::\n");
                break;
            } else {
                if (save_message(buf, actual_args->conn)) {
                    printf("Error: %s [%d]\n", mysql_error(actual_args->conn), mysql_errno(actual_args->conn));
                }
            }
        }
    }
}

int send_to_client_all_messages(int connect_d, MYSQL *conn) {
    MYSQL_RES *res;
    MYSQL_ROW row;
    get_all_rows(conn, "messages");
    res = mysql_store_result(conn);
    while ((row = mysql_fetch_row(res))) {
        say(connect_d, row[1]);
        say(connect_d, "\r\n");
    }

    mysql_free_result(res);

    return 0;
}

void send_all_clients(char *msg, node_t *list, int connect_d) {
    while (list->next != NULL) {
        list = list->next;
        if (list->val != connect_d) {
            say(list->val, msg);
            say(list->val, "\r\n");
        }
    }
}

size_t read_in(int socket, char *buf, size_t len) {
    char *s = buf;
    size_t slen = len;
    ssize_t c = recv(socket, s, slen, 0);
    slen -= c;
    while ((c > 0) && (s[c - 1] != '\n')) {
        s += c;
        slen -= c;
        c = recv(socket, s, slen, 0);
    }
    if (c < 0) {
        printf("An error occurred while receiving the message\n");
    } else {
        if (c == 0) {
            buf[0] = '\0';
        } else {
            s[c - 1] = '\0';
            ++slen;
        }
    }
    return len - slen;
}

int connect_client(int socket) {
    struct sockaddr_storage client_addr;
    unsigned int address_size = sizeof(client_addr);
    int connect_d = accept(socket, (struct sockaddr *) &client_addr, &address_size);
    if (connect_d == -1) {
        error("Can't open second socket");
    } else {
        printf("Opened second socket\n");
    }
    return connect_d;
}

void set_limit_listeners(int socket) {
    if (listen(socket, limit_listeners) == -1) {
        error("Can't listen port");
    } else {
        printf("Listen port: %i\n", port);
    }
}

int open_listener_socket() {
    int s = socket(PF_INET, SOCK_STREAM, 0);

    if (s == -1) {
        error("Can't open socket");
    } else {
        printf("Socket opened\n");
    }

    return s;
}

void bind_to_port(int socket) {
    int reuse = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse, sizeof(int))) {
        error("Can't set reuse param");
    } else {
        printf("Set reuse param\n");
    }

    struct sockaddr_in name;
    name.sin_family = PF_INET;
    name.sin_port = (in_port_t) htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    int c = bind(socket, (struct sockaddr *) &name, sizeof(name));
    if (c == -1) {
        error("Can't bind to socket");
    } else {
        printf("Bound to socket\n");
    }
}

ssize_t say(int socket, char *s) {
    ssize_t result = send(socket, s, strlen(s), 0);
    if (result == -1) {
        printf("An error occurred while sending the message\n");
    } else {
        printf("Sent the message\n");
    }
    return result;
}

void error(char *msg) {
    printf("%s\n", msg);
    exit(1);
}