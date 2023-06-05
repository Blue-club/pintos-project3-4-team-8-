#include <stdio.h>
#include <string.h>

#define MAX_STR_LEN 100  // 각 문자열의 최대 길이
#define MAX_NUM_STR 10   // 문자열 배열의 최대 개수



int tktk(char *ttt) {
    char *save_token;
    char str[MAX_NUM_STR][MAX_STR_LEN];
    int token_nums = 0;


    char *token = strtok_r(ttt, " ", &save_token);

    while(token != NULL) {
        
        strcpy(str[token_nums], token);

        token_nums++;
        token = strtok_r(NULL, " ", &save_token);
    }

    for (int i = 0; i < token_nums; i++) {
        printf("%p\n", &str[i]);
    }

    return 0;
}

int main() {
    char org_str[11] = "echo x y z";


    tktk(org_str);


    return 0;
}
