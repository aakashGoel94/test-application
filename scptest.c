#include<libssh/libssh.h>
#include<stdlib.h>
#include<stdio.h>
#include<errno.h>
#include<string.h>

int verify_knownhost(ssh_session session);
int scp_write(ssh_session session);
int scp_read(ssh_session session);
int scp_helloworld(ssh_session session, ssh_scp scp);
int scp_receive(ssh_session session, ssh_scp scp);

int main()
{
  ssh_session my_ssh_session;
  int verbosity = SSH_LOG_PROTOCOL;
  int rc;
  char *password;
 
  // Open session and set options
  my_ssh_session = ssh_new();
  if (my_ssh_session == NULL)
    exit(-1);
  ssh_options_set(my_ssh_session, SSH_OPTIONS_HOST, "localhost");
  ssh_options_set(my_ssh_session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
  

  // Connect to server
  rc = ssh_connect(my_ssh_session);
  if (rc != SSH_OK)
  {
    fprintf(stderr, "Error connecting to localhost: %s\n",
            ssh_get_error(my_ssh_session));
    ssh_free(my_ssh_session);
    exit(-1);
  }
 
  // Verify the server's identity
  // For the source code of verify_knownhost(), check previous example
  if (verify_knownhost(my_ssh_session) < 0)
  {
    ssh_disconnect(my_ssh_session);
    ssh_free(my_ssh_session);
    exit(-1);
  }
 
  // Authenticate ourselves
  password = getpass("Password: ");
  rc = ssh_userauth_password(my_ssh_session, NULL, password);
  if (rc != SSH_AUTH_SUCCESS)
  {
    fprintf(stderr, "Error authenticating with password: %s\n",
            ssh_get_error(my_ssh_session));
    ssh_disconnect(my_ssh_session);
    ssh_free(my_ssh_session);
    exit(-1);
  }
  
  scp_write(my_ssh_session);
  scp_read(my_ssh_session);

  ssh_disconnect(my_ssh_session);
  ssh_free(my_ssh_session);
}

int verify_knownhost(ssh_session session)
{
    enum ssh_known_hosts_e state;
    unsigned char *hash = NULL;
    ssh_key srv_pubkey = NULL;
    size_t hlen;
    char buf[50];
    char *hexa;
    char *p;
    int cmp;
    int rc;
 
    rc = ssh_get_server_publickey(session, &srv_pubkey);
    if (rc < 0) {
        return -1;
    }
 
    rc = ssh_get_publickey_hash(srv_pubkey,
                                SSH_PUBLICKEY_HASH_SHA1,
                                &hash,
                                &hlen);
    ssh_key_free(srv_pubkey);
    if (rc < 0) {
        return -1;
    }
 
    state = ssh_session_is_known_server(session);
    switch (state) {
        case SSH_KNOWN_HOSTS_OK:
            /* OK */
 
            break;
        case SSH_KNOWN_HOSTS_CHANGED:
            fprintf(stderr, "Host key for server changed: it is now:\n");
            ssh_print_hexa("Public key hash", hash, hlen);
            fprintf(stderr, "For security reasons, connection will be stopped\n");
            ssh_clean_pubkey_hash(&hash);
 
            return -1;
        case SSH_KNOWN_HOSTS_OTHER:
            fprintf(stderr, "The host key for this server was not found but an other"
                    "type of key exists.\n");
            fprintf(stderr, "An attacker might change the default server key to"
                    "confuse your client into thinking the key does not exist\n");
            ssh_clean_pubkey_hash(&hash);
 
            return -1;
        case SSH_KNOWN_HOSTS_NOT_FOUND:
            fprintf(stderr, "Could not find known host file.\n");
            fprintf(stderr, "If you accept the host key here, the file will be"
                    "automatically created.\n");
 
            /* FALL THROUGH to SSH_SERVER_NOT_KNOWN behavior */
 
        case SSH_KNOWN_HOSTS_UNKNOWN:
            hexa = ssh_get_hexa(hash, hlen);
            fprintf(stderr,"The server is unknown. Do you trust the host key?\n");
            fprintf(stderr, "Public key hash: %s\n", hexa);
            ssh_string_free_char(hexa);
            ssh_clean_pubkey_hash(&hash);
            p = fgets(buf, sizeof(buf), stdin);
            if (p == NULL) {
                return -1;
            } 
 
            cmp = strncasecmp(buf, "yes", 3);
            if (cmp != 0) {
                return -1;
            }
 
            rc = ssh_session_update_known_hosts(session);
            if (rc < 0) {
                fprintf(stderr, "Error %s\n", strerror(errno));
                return -1;
            }
 
            break;
        case SSH_KNOWN_HOSTS_ERROR:
            fprintf(stderr, "Error %s", ssh_get_error(session));
            ssh_clean_pubkey_hash(&hash);
            return -1;
    }
 
    ssh_clean_pubkey_hash(&hash);
    return 0;
}

int scp_write(ssh_session session)
{
  ssh_scp scp;
  int rc;
 
  scp = ssh_scp_new
    (session, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, "/home/aakash/Downloads");
  if (scp == NULL)
  {
    fprintf(stderr, "Error allocating scp session: %s\n",
            ssh_get_error(session));
    return SSH_ERROR;
  }
 
  rc = ssh_scp_init(scp);
  if (rc != SSH_OK)
  {
    fprintf(stderr, "Error initializing scp session: %s\n",
            ssh_get_error(session));
    ssh_scp_free(scp);
    return rc;
  }
  scp_helloworld(session, scp);
  ssh_scp_close(scp);
  ssh_scp_free(scp);
  return SSH_OK;
}
  
int scp_helloworld(ssh_session session, ssh_scp scp)
{
  int rc;
  const char *helloworld = "Hello, world!\n";
  int length = strlen(helloworld);
 
  rc = ssh_scp_push_directory(scp, "helloworld", 777);
  if (rc != SSH_OK)
  {
    fprintf(stderr, "Can't create remote directory: %s\n",
            ssh_get_error(session));
    return rc;
  }
 
  rc = ssh_scp_push_file
    (scp, "helloworld.txt", length, 777);
  if (rc != SSH_OK)
  {
    fprintf(stderr, "Can't open remote file: %s\n",
            ssh_get_error(session));
    return rc;
  }
 
  rc = ssh_scp_write(scp, helloworld, length);
  if (rc != SSH_OK)
  {
    fprintf(stderr, "Can't write to remote file: %s\n",
            ssh_get_error(session));
    return rc;
  }
 
  return SSH_OK;
}

int scp_read(ssh_session session)
{
  ssh_scp scp;
  int rc;
 
  scp = ssh_scp_new
    (session, SSH_SCP_READ, "helloworld/helloworld.txt");
  if (scp == NULL)
  {
    fprintf(stderr, "Error allocating scp session: %s\n",
            ssh_get_error(session));
    return SSH_ERROR;
  }
 
  rc = ssh_scp_init(scp);
  if (rc != SSH_OK)
  {
    fprintf(stderr, "Error initializing scp session: %s\n",
            ssh_get_error(session));
    ssh_scp_free(scp);
    return rc;
  }
 
  //scp_receive(session, scp);
  ssh_scp_close(scp);
  ssh_scp_free(scp);
  return SSH_OK;
}

int scp_receive(ssh_session session, ssh_scp scp)
{
  int rc;
  int size, mode;
  char *filename, *buffer;
 
  rc = ssh_scp_pull_request(scp);
  if (rc != SSH_SCP_REQUEST_NEWFILE)
  {
    fprintf(stderr, "Error receiving information about file: %s\n",
            ssh_get_error(session));
    return SSH_ERROR;
  }
 
  size = ssh_scp_request_get_size(scp);
  filename = strdup(ssh_scp_request_get_filename(scp));
  mode = ssh_scp_request_get_permissions(scp);
  printf("Receiving file %s, size %d, permissions 0%o\n",
          filename, size, mode);
  free(filename);
 
  buffer = malloc(size);
  if (buffer == NULL)
  {
    fprintf(stderr, "Memory allocation error\n");
    return SSH_ERROR;
  }
 
  ssh_scp_accept_request(scp);
  rc = ssh_scp_read(scp, buffer, size);
  if (rc == SSH_ERROR)
  {
    fprintf(stderr, "Error receiving file data: %s\n",
            ssh_get_error(session));
    free(buffer);
    return rc;
  }
  printf("Done\n");
 
  write(1, buffer, size);
  free(buffer);
 
  rc = ssh_scp_pull_request(scp);
  if (rc != SSH_SCP_REQUEST_EOF)
  {
    fprintf(stderr, "Unexpected request: %s\n",
            ssh_get_error(session));
    return SSH_ERROR;
  }
 
  return SSH_OK;
}

