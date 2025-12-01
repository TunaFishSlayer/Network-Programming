#include "data_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Định nghĩa file lưu trữ
#define USER_FILE "users.txt"
#define FILES_FILE "shared_files.txt"
#define SESSION_TIMEOUT 3600  // 1 giờ

// Định nghĩa các biến global
User* users = NULL;
SharedFile* files = NULL;
Session* sessions = NULL;
pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t files_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t sessions_mutex = PTHREAD_MUTEX_INITIALIZER;

void ensure_data_files_exist() {
    FILE* fp;

    // Kiểm tra và tạo file users.txt nếu không tồn tại
    fp = fopen(USER_FILE, "r");
    if (!fp) {
        fp = fopen(USER_FILE, "w");
        if (fp) {
            fclose(fp);
        }
    } else {
        fclose(fp);
    }

    // Kiểm tra và tạo file shared_files.txt nếu không tồn tại
    fp = fopen(FILES_FILE, "r");
    if (!fp) {
        fp = fopen(FILES_FILE, "w");
        if (fp) {
            fclose(fp);
        }
    } else {
        fclose(fp);
    }
}

// ----------------------------------------------------------------
//                          CHỨC NĂNG LƯU DỮ LIỆU
// ----------------------------------------------------------------

void save_users() {
    pthread_mutex_lock(&users_mutex);
    FILE* fp = fopen(USER_FILE, "w"); // Ghi đè
    if (!fp) {
        perror("Lỗi khi mở users.txt để ghi");
        pthread_mutex_unlock(&users_mutex);
        return;
    }

    User* current = users;
    while (current) {
        // Định dạng: email|username|password\n
        fprintf(fp, "%s|%s|%s\n", current->email, current->username, current->password);
        current = current->next;
    }

    fclose(fp);
    pthread_mutex_unlock(&users_mutex);
}

void save_shared_files() {
    pthread_mutex_lock(&files_mutex);
    FILE* fp = fopen(FILES_FILE, "w"); // Ghi đè
    if (!fp) {
        perror("Lỗi khi mở shared_files.txt để ghi");
        pthread_mutex_unlock(&files_mutex);
        return;
    }

    SharedFile* current = files;
    while (current) {
        // Định dạng: filename|hash|email|ip|port|size|chunk_size\n
        fprintf(fp, "%s|%s|%s|%s|%d|%ld|%d\n",
                current->filename, current->filehash, current->owner_email,
                current->ip, current->port, current->file_size, current->chunk_size);
        current = current->next;
    }

    fclose(fp);
    pthread_mutex_unlock(&files_mutex);
}

// ----------------------------------------------------------------
//                          CHỨC NĂNG TẢI DỮ LIỆU
// ----------------------------------------------------------------

void load_users() {
    pthread_mutex_lock(&users_mutex);
    FILE* fp = fopen(USER_FILE, "r");
    if (!fp) {
        pthread_mutex_unlock(&users_mutex);
        return;
    }

    char line[MAX_EMAIL + MAX_USERNAME + MAX_PASSWORD + 5];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;

        char* email = strtok(line, "|");
        char* username = strtok(NULL, "|");
        char* password = strtok(NULL, "|");

        if (email && username && password) {
            User* new_user = (User*)malloc(sizeof(User));
            strcpy(new_user->email, email);
            strcpy(new_user->username, username);
            strcpy(new_user->password, password);
            new_user->next = users;
            users = new_user;
        }
    }

    fclose(fp);
    pthread_mutex_unlock(&users_mutex);
}

void load_shared_files() {
    pthread_mutex_lock(&files_mutex);
    FILE* fp = fopen(FILES_FILE, "r");
    if (!fp) {
        pthread_mutex_unlock(&files_mutex);
        return;
    }

    char line[MAX_FILENAME + MAX_HASH + MAX_EMAIL + MAX_IP + 50];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        char* filename = strtok(line, "|");
        char* filehash = strtok(NULL, "|");
        char* owner_email = strtok(NULL, "|");
        char* ip = strtok(NULL, "|");
        char* port_str = strtok(NULL, "|");
        char* file_size_str = strtok(NULL, "|");
        char* chunk_size_str = strtok(NULL, "|");

        if (filename && filehash && owner_email && ip && port_str && file_size_str && chunk_size_str) {
            SharedFile* new_file = (SharedFile*)malloc(sizeof(SharedFile));
            strcpy(new_file->filename, filename);
            strcpy(new_file->filehash, filehash);
            strcpy(new_file->owner_email, owner_email);
            strcpy(new_file->ip, ip);
            new_file->port = atoi(port_str);
            new_file->file_size = atol(file_size_str);
            new_file->chunk_size = atoi(chunk_size_str);
            new_file->next = files;
            files = new_file;
        }
    }

    fclose(fp);
    pthread_mutex_unlock(&files_mutex);
}

void load_data() {
    load_users();
    load_shared_files();
}

// ----------------------------------------------------------------
//                          CHỨC NĂNG LOGIC SERVER
// ----------------------------------------------------------------

int get_username_by_email(const char* email, char* username_out) {
    pthread_mutex_lock(&users_mutex);
    
    User* current = users;
    while (current) {
        if (strcmp(current->email, email) == 0) {
            strcpy(username_out, current->username);
            pthread_mutex_unlock(&users_mutex);
            return 1;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&users_mutex);
    return 0;
}

int add_user(const char* email, const char* username, const char* password) {
    pthread_mutex_lock(&users_mutex);
    
    User* current = users;
    while (current) {
        if (strcmp(current->email, email) == 0) {
            pthread_mutex_unlock(&users_mutex);
            return 0; // Email đã tồn tại
        }
        current = current->next;
    }
    
    User* new_user = (User*)malloc(sizeof(User));
    strcpy(new_user->email, email);
    strcpy(new_user->username, username);
    strcpy(new_user->password, password);
    new_user->next = users;
    users = new_user;
    
    pthread_mutex_unlock(&users_mutex);
    save_users(); // Lưu lại sau khi thêm
    return 1;
}

int authenticate(const char* email, const char* password) {
    pthread_mutex_lock(&users_mutex);
    
    User* current = users;
    while (current) {
        if (strcmp(current->email, email) == 0 &&
            strcmp(current->password, password) == 0) {
            pthread_mutex_unlock(&users_mutex);
            return 1;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&users_mutex);
    return 0;
}

void publish_file(const char* filename, const char* filehash, const char* owner_email,
                  const char* ip, int port, long file_size, int chunk_size) {
    pthread_mutex_lock(&files_mutex);
    
    // Kiểm tra file đã tồn tại chưa (cùng hash và cùng peer)
    SharedFile* current = files;
    while (current) {
        if (strcmp(current->filehash, filehash) == 0 &&
            strcmp(current->owner_email, owner_email) == 0) {
            // Cập nhật thông tin peer (IP/Port) nếu cần
            strcpy(current->ip, ip);
            current->port = port;
            pthread_mutex_unlock(&files_mutex);
            return;
        }
        current = current->next;
    }
    
    SharedFile* new_file = (SharedFile*)malloc(sizeof(SharedFile));
    strcpy(new_file->filename, filename);
    strcpy(new_file->filehash, filehash);
    strcpy(new_file->owner_email, owner_email);
    strcpy(new_file->ip, ip);
    new_file->port = port;
    new_file->file_size = file_size;
    new_file->chunk_size = chunk_size;
    new_file->next = files;
    files = new_file;
    
    pthread_mutex_unlock(&files_mutex);
    save_shared_files(); // Lưu lại sau khi công bố
}

int unpublish_file(const char* filehash, const char* owner_email) {
    pthread_mutex_lock(&files_mutex);
    
    SharedFile* current = files;
    SharedFile* prev = NULL;
    int file_removed = 0;
    
    while (current) {
        if (strcmp(current->filehash, filehash) == 0 &&
            strcmp(current->owner_email, owner_email) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                files = current->next;
            }
            SharedFile* to_delete = current;
            current = current->next;
            free(to_delete);
            file_removed = 1;
        } else {
            prev = current;
            current = current->next;
        }
    }
    
    pthread_mutex_unlock(&files_mutex);
    if (file_removed) {
        save_shared_files(); // Lưu lại sau khi hủy công bố
    }
    
    return file_removed;
}

// ================================================================
//              SESSION & VERIFICATION FUNCTIONS
// ================================================================

char* create_session(const char* email) {
    pthread_mutex_lock(&sessions_mutex);
    
    // Generate simple token (in real system, use UUID or crypto)
    static char token[64];
    snprintf(token, sizeof(token), "token_%ld_%s", time(NULL), email);
    
    Session* new_session = (Session*)malloc(sizeof(Session));
    strcpy(new_session->token, token);
    strcpy(new_session->email, email);
    new_session->login_time = time(NULL);
    new_session->next = sessions;
    sessions = new_session;
    
    pthread_mutex_unlock(&sessions_mutex);
    
    char* result = (char*)malloc(64);
    strcpy(result, token);
    return result;
}

int verify_token(const char* token, const char* email) {
    if (!token || strlen(token) == 0) {
        return 0;
    }
    
    pthread_mutex_lock(&sessions_mutex);
    
    Session* current = sessions;
    while (current) {
        if (strcmp(current->token, token) == 0 && 
            strcmp(current->email, email) == 0) {
            
            // Check if session expired
            time_t now = time(NULL);
            if (now - current->login_time > SESSION_TIMEOUT) {
                pthread_mutex_unlock(&sessions_mutex);
                return 0;  // Session expired
            }
            
            pthread_mutex_unlock(&sessions_mutex);
            return 1;  // Valid token
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&sessions_mutex);
    return 0;  // Token not found
}

void destroy_session(const char* token) {
    pthread_mutex_lock(&sessions_mutex);
    
    Session* current = sessions;
    Session* prev = NULL;
    
    while (current) {
        if (strcmp(current->token, token) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                sessions = current->next;
            }
            free(current);
            break;
        }
        prev = current;
        current = current->next;
    }
    
    pthread_mutex_unlock(&sessions_mutex);
}

int is_file_owner(const char* filehash, const char* email) {
    pthread_mutex_lock(&files_mutex);
    
    SharedFile* current = files;
    while (current) {
        if (strcmp(current->filehash, filehash) == 0) {
            int is_owner = (strcmp(current->owner_email, email) == 0);
            pthread_mutex_unlock(&files_mutex);
            return is_owner;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&files_mutex);
    return 0;  // File not found
}

int validate_email(const char* email) {
    if (!email || strlen(email) == 0 || strlen(email) >= MAX_EMAIL) {
        return 0;
    }
    
    // Simple email validation - check for @ symbol
    return strchr(email, '@') != NULL;
}

int validate_filename(const char* filename) {
    if (!filename || strlen(filename) == 0 || strlen(filename) >= MAX_FILENAME) {
        return 0;
    }
    
    // Check for path traversal attempts
    if (strstr(filename, "..") != NULL || strchr(filename, '/') != NULL) {
        return 0;
    }
    
    return 1;
}

SearchResponse search_files(const char* keyword) {
    SearchResponse response;
    response.response_code = RESP_SUCCESS;
    response.count = 0;
    
    pthread_mutex_lock(&files_mutex);
    
    SharedFile* current = files;
    char seen_hashes[100][MAX_HASH];
    int seen_count = 0;
    
    while (current && response.count < 100) {
        printf("DEBUG SEARCH: So sánh '%s' với từ khóa '%s'\n", current->filename, keyword);
        if (strstr(current->filename, keyword) != NULL) {
            
            // Chỉ thêm hash nếu chưa thấy
            int already_added = 0;
            for (int i = 0; i < seen_count; i++) {
                if (strcmp(seen_hashes[i], current->filehash) == 0) {
                    already_added = 1;
                    break;
                }
            }
            
            if (!already_added) {
                strcpy(response.files[response.count].filename, current->filename);
                strcpy(response.files[response.count].filehash, current->filehash);
                response.files[response.count].file_size = current->file_size;
                response.files[response.count].chunk_size = current->chunk_size;
                
                strcpy(seen_hashes[seen_count], current->filehash);
                seen_count++;
                response.count++;
            }
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&files_mutex);
    
    if (response.count == 0) {
        response.response_code = RESP_NOT_FOUND;
    }
    
    return response;
}

FindResponse find_peers(const char* filehash) {
    FindResponse response;
    response.response_code = RESP_SUCCESS;
    response.count = 0;
    
    pthread_mutex_lock(&files_mutex);
    SharedFile* current = files;
    
    // Track unique peers (by ip:port)
    char seen_peers[50][MAX_IP + 6]; 
    int seen_count = 0;
    
    while (current && response.count < 50) {
        if (strcmp(current->filehash, filehash) == 0) {
            
            char peer_key[MAX_IP + 6];
            snprintf(peer_key, sizeof(peer_key), "%s:%d", current->ip, current->port);
            
            int already_added = 0;
            for(int i = 0; i < seen_count; i++) {
                if (strcmp(seen_peers[i], peer_key) == 0) {
                    already_added = 1;
                    break;
                }
            }
            
            if (!already_added) {
                strcpy(response.peers[response.count].ip, current->ip);
                response.peers[response.count].port = current->port;
                
                strcpy(seen_peers[seen_count], peer_key);
                seen_count++;
                response.count++;
            }
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&files_mutex);
    
    if (response.count == 0) {
        response.response_code = RESP_NOT_FOUND;
    }
    
    return response;
}