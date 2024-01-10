#define MAXLINE 1024
#define IMAGE_DIR "images/"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>  // Used to inspect directory contents.
#include "response.h"
#include "request.h"
#include <fcntl.h>
#include <sys/stat.h>


// Functions for internal use only.
void write_image_list(int fd);
void write_image_response_header(int fd);


/*
 * Write the main.html response to the given fd.
 * This response dynamically populates the image-filter form with
 * the filenames located in IMAGE_DIR.
 */
void main_html_response(int fd) {
    char *header =
        "HTTP/1.1 200 OK\r\n"
        "Content-type: text/html\r\n\r\n";

    if(write(fd, header, strlen(header)) == -1) {
        perror("write");
    }

    FILE *in_fp = fopen("main.html", "r");
    char buf[MAXLINE];
    while (fgets(buf, MAXLINE, in_fp) > 0) {
        if(write(fd, buf, strlen(buf)) == -1) {
            perror("write");
        }
        // Insert a bit of dynamic Javascript into the HTML page.
        // This assumes there's only one "<script>" element in the page.
        if (strncmp(buf, "<script>", strlen("<script>")) == 0) {
            write_image_list(fd);
        }
    }
    fclose(in_fp);
}


/*
 * Write image directory contents to the given fd, in the format
 * "var filenames = ['<filename1>', '<filename2>', ...];\n"
 *
 * This is actually a line of Javascript that's used to populate the form
 * when the webpage is loaded.
 */
void write_image_list(int fd) {
    DIR *d = opendir(IMAGE_DIR);
    struct dirent *dir;

    dprintf(fd, "var filenames = [");
    if (d != NULL) {
        while ((dir = readdir(d)) != NULL) {
            if (strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
                dprintf(fd, "'%s', ", dir->d_name);
            }
        }
        closedir(d);
    }
    dprintf(fd, "];\n");
}


/*
 * Given the socket fd and request data, do the following:
 * 1. Determine whether the request is valid according to the conditions
 *    under the "Input validation" section of Part 3 of the handout.
 *
 *    Ignore all other query parameters, and any other data in the request.
 *    Read about and use the "access" system call to check for the presence of
 *    files *with the correct permissions*.
 *
 * 2. If the request is invalid, send an informative error message as a response
 *    using the internal_server_error_response function.
 *
 * 3. Otherwise, write an appropriate HTTP header for a bitmap file (we've
 *    provided a function to do so), and then use dup2 and execl to run
 *    the specified image filter and write the output directly to the socket.
 */
void image_filter_response(int fd, const ReqData *reqData) {
    // check for validity 1
    int err = 0;
    if (strcmp(reqData->params[0].name, "filter") != 0 && strcmp(reqData->params[1].name, "filter") != 0) {
        internal_server_error_response(fd, "Missing filter parameter.\n");
        err = 1;
    } 
    else {
        for (int f_index = 0; f_index < 2; f_index ++) {
            if (strcmp(reqData->params[f_index].name, "filter") == 0) {
                // check for validity 3
                char *f_value = reqData->params[f_index].value;
                int f_len = strlen(FILTER_DIR)+strlen(f_value);
                char *filtername = malloc(sizeof(char) * (strlen(FILTER_DIR)+strlen(f_value)+1));
                strcpy(filtername, FILTER_DIR);
                strcat(filtername, f_value);
                filtername[f_len] = '\0';
                struct stat *filter_stat = malloc(sizeof(struct stat));
                if (stat(filtername, filter_stat) != 0 || !S_ISREG(filter_stat->st_mode)) {
                    internal_server_error_response(fd, "No such filter is found.\n");
                    err = 1;
                } else if (access(filtername, X_OK) != 0) {
                    internal_server_error_response(fd, "Such filter is not executable\n");
                    err = 1;
                }
                free(filtername);
                free(filter_stat);
            }
        }
    }

    // check for validity 2
    if (strcmp(reqData->params[1].name, "image") != 0 && strcmp(reqData->params[0].name, "image") != 0) {
        internal_server_error_response(fd, "Missing image parameter.\n");
        err = 1;
    } 
    else {
        for (int i_index = 0; i_index < 2; i_index++) {
            if (strcmp(reqData->params[i_index].name, "image") == 0) {
                // check for validity 4
                char *i_value = reqData->params[i_index].value;
                int i_len = strlen(FILTER_DIR)+strlen(i_value);
                char *imagename = malloc(sizeof(char) * (i_len+1));
                strcpy(imagename, IMAGE_DIR);
                strcat(imagename, i_value);
                imagename[i_len] = '\0';
                struct stat *image_stat = malloc(sizeof(struct stat));
                if (stat(imagename, image_stat) != 0 || !S_ISREG(image_stat->st_mode)) {
                    internal_server_error_response(fd, "No such image is found.\n");
                    err = 1;
                } else if (access(imagename, R_OK) != 0) {
                    internal_server_error_response(fd, "Such image is not readable\n");
                    err = 1;
                }
                free(imagename);
                free(image_stat);
            }
        }
    }

    // check for validity 2
    if ((strchr(reqData->params[0].value, '\\') != NULL)) {
        internal_server_error_response(fd, "Backslash observed in first value.\n");
        err = 1;
    }
    if ((strchr(reqData->params[1].value, '\\') != NULL)) {
        internal_server_error_response(fd, "Backslash observed in second value.\n");
        err = 1;
    }

    if (err == 1) {
        return;
    }
    else {
        int f, i;
        if (strcmp(reqData->params[0].name, "image") == 0) {
            f = 1;
            i = 0;
        } else {
            f = 0;
            i = 1;
        }
        char imagename[strlen(IMAGE_DIR)+strlen(reqData->params[i].value)+1];
        char filtername[strlen(FILTER_DIR)+strlen(reqData->params[f].value)+1];
        char executable[strlen(reqData->params[f].value)+1];
        strcpy(imagename, IMAGE_DIR);
        strcat(imagename, reqData->params[i].value);
        imagename[strlen(IMAGE_DIR)+strlen(reqData->params[i].value)] = '\0';
        strcpy(filtername, FILTER_DIR);
        strcat(filtername, reqData->params[f].value);
        filtername[strlen(FILTER_DIR)+strlen(reqData->params[f].value)] ='\0';
        strcpy(executable, reqData->params[f].value);
        executable[strlen(reqData->params[f].value)] = '\0';

        write_image_response_header(fd);
        int in_fd;
        if ((in_fd = open(imagename, O_RDONLY)) == -1) {
            perror("fopen");
            exit(1);
        }
        if (dup2(in_fd, STDIN_FILENO) == -1) {
            perror("dup2");
            exit(1);
        }
        if (dup2(fd, STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(1);
        }
        close(in_fd);
        close(fd);
        execl(filtername, executable, NULL);
    }
}


/*
 * Respond to an image-upload request.
 * We have provided the complete implementation of this function;
 * you shouldn't change it, but instead read through it carefully and implement
 * the required functions in `request.c` according to their docstrings.
 *
 * We've split up the parsing of the rest of the request data into different
 * steps, so at each step it's a bit easier for you to test your code.
 */
void image_upload_response(ClientState *client) {
    // First, extract the boundary string for the request.
    char *boundary = get_boundary(client);
    if (boundary == NULL) {
        bad_request_response(client->sock, "Couldn't find boundary string in request.");
        exit(1);
    }
    fprintf(stderr, "Boundary string: %s\n", boundary);

    // Use the boundary string to extract the name of the uploaded bitmap file.
    char *filename = get_bitmap_filename(client, boundary);
    if (filename == NULL) {
        bad_request_response(client->sock, "Couldn't find bitmap filename in request.");
        close(client->sock);
        exit(1);
    }

    // If the file already exists, send a Bad Request error to the user.
    char *path = malloc(strlen(IMAGE_DIR) + strlen(filename) + 1);
    strcpy(path, IMAGE_DIR);
    strcat(path, filename);

    fprintf(stderr, "Bitmap path: %s\n", path);

    if (access(path, F_OK) >= 0) {
        bad_request_response(client->sock, "File already exists.");
        exit(1);
    }

    FILE *file = fopen(path, "wb");
    save_file_upload(client, boundary, fileno(file));
    fclose(file);
    free(boundary);
    free(filename);
    free(path);
    see_other_response(client->sock, MAIN_HTML);
}


/*
 * Write the header for a bitmap image response to the given fd.
 */
void write_image_response_header(int fd) {
    char *response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: image/bmp\r\n"
        "Content-Disposition: attachment; filename=\"output.bmp\"\r\n\r\n";

    write(fd, response, strlen(response));
}


void not_found_response(int fd) {
    char *response =
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/plain\r\n\r\n"
        "Page not found.\r\n";
    write(fd, response, strlen(response));
}


void internal_server_error_response(int fd, const char *message) {
    char *response =
        "HTTP/1.1 500 Internal Server Error\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
        "<html><head>\r\n"
        "<title>500 Internal Server Error</title>\r\n"
        "</head><body>\r\n"
        "<h1>Internal Server Error</h1>\r\n"
        "<p>%s<p>\r\n"
        "</body></html>\r\n";

    dprintf(fd, response, message);
}


void bad_request_response(int fd, const char *message) {
    char *response_header =
        "HTTP/1.1 400 Bad Request\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n\r\n";
    char *response_body = 
        "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
        "<html><head>\r\n"
        "<title>400 Bad Request</title>\r\n"
        "</head><body>\r\n"
        "<h1>Bad Request</h1>\r\n"
        "<p>%s<p>\r\n"
        "</body></html>\r\n";
    char header_buf[MAXLINE];
    char body_buf[MAXLINE];
    sprintf(body_buf, response_body, message);
    sprintf(header_buf, response_header, strlen(body_buf));
    write(fd, header_buf, strlen(header_buf));
    write(fd, body_buf, strlen(body_buf));
    // Because we are making some simplfications with the HTTP protocol
    // the browser will get a "connection reset" message. This happens
    // because our server is closing the connection and terminating the process.
    // So this is really a hack.
    sleep(1);
}


void see_other_response(int fd, const char *other) {
    char *response =
        "HTTP/1.1 303 See Other\r\n"
        "Location: %s\r\n\r\n";

    dprintf(fd, response, other);
}
