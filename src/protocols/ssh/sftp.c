
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac-client-ssh.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */


#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>

#include <libssh/libssh.h>
#include <libssh/sftp.h>

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/stream.h>

#include "client.h"

static bool __ssh_guac_valid_filename(char* filename) {

    /* Disallow "." as a filename */
    if (strcmp(filename, ".") == 0)
        return false;

    /* Disallow ".." as a filename */
    if (strcmp(filename, "..") == 0)
        return false;

    /* Search for path separator characters */
    for (;;) {

        char c = *(filename++);
        if (c == '\0')
            break;

        /* Replace slashes with underscores */
        if (c == '\\' || c == '/')
            return false;

    }

    /* If filename does not contain a path, it's ok */
    return true;

}

int guac_sftp_file_handler(guac_client* client, guac_stream* stream,
        char* mimetype, char* filename) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    sftp_file file;

    /* Ensure filename is a valid filename and not a path */
    if (!__ssh_guac_valid_filename(filename)) {
        guac_protocol_send_ack(client->socket, stream,
                "SFTP: Illegal filename",
                GUAC_PROTOCOL_STATUS_INVALID_PARAMETER);
        guac_socket_flush(client->socket);
        return 0;
    }

    /* Open file via SFTP */
    file = sftp_open(client_data->sftp_session, filename,
            O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    /* Inform of status */
    if (file != NULL) {
        guac_protocol_send_ack(client->socket, stream, "SFTP: File opened",
                GUAC_PROTOCOL_STATUS_SUCCESS);
        guac_socket_flush(client->socket);
    }
    else {
        guac_client_log_error(client, "Unable to open file: %s",
                ssh_get_error(client_data->session));
        guac_protocol_send_ack(client->socket, stream, "SFTP: Open failed",
                GUAC_PROTOCOL_STATUS_INTERNAL_ERROR);
        guac_socket_flush(client->socket);
    }

    /* Store file within stream */
    stream->data = file;
    return 0;

}

int guac_sftp_blob_handler(guac_client* client, guac_stream* stream,
        void* data, int length) {

    /* Pull file from stream */
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    sftp_file file = (sftp_file) stream->data;

    /* Attempt write */
    if (sftp_write(file, data, length) == length) {
        guac_protocol_send_ack(client->socket, stream, "SFTP: OK",
                GUAC_PROTOCOL_STATUS_SUCCESS);
        guac_socket_flush(client->socket);
    }

    /* Inform of any errors */
    else {
        guac_client_log_error(client, "Unable to write to file: %s",
                ssh_get_error(client_data->session));
        guac_protocol_send_ack(client->socket, stream, "SFTP: Write failed",
                GUAC_PROTOCOL_STATUS_INTERNAL_ERROR);
        guac_socket_flush(client->socket);
    }

    return 0;

}

int guac_sftp_end_handler(guac_client* client, guac_stream* stream) {

    /* Pull file from stream */
    sftp_file file = (sftp_file) stream->data;

    /* Attempt to close file */
    if (sftp_close(file) == SSH_OK) {
        guac_protocol_send_ack(client->socket, stream, "SFTP: OK",
                GUAC_PROTOCOL_STATUS_SUCCESS);
        guac_socket_flush(client->socket);
    }
    else {
        guac_client_log_error(client, "Unable to close file");
        guac_protocol_send_ack(client->socket, stream, "SFTP: Close failed",
                GUAC_PROTOCOL_STATUS_INTERNAL_ERROR);
        guac_socket_flush(client->socket);
    }

    return 0;

}

guac_stream* guac_sftp_download_file(guac_client* client, const char* filename) {
    /* STUB */
    return NULL;
}

