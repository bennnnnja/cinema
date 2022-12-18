#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <regex.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include "termcolor-c.h"

typedef struct Film {
    char title[96];
    int year;
    char countries[64];
    char genres[64];
    float rating;

    struct Film *prev;
    struct Film *next;
} Film;

typedef struct Films {
    int size;
    Film *current;
} Films;

typedef struct {
    char login[24];
    char password[24];
    char card[20];
    Films *favorites;
    int is_admin;
} User;

// Создание кольцевого списка
Films *create_films_ring() {
    Films *films = (Films *)malloc(sizeof(Films));
    films->size = 0;
    films->current = NULL;
    return films;
}

// Добавление нового фильма в список
Film *add_film(Films *ring) {
    Film *film = (Film *)malloc(sizeof(Film));

    if (ring->current == NULL) {
        ring->current = film;
        film->prev = film;
        film->next = film;
    } else {
        film->prev = ring->current->prev;
        film->next = ring->current;
        ring->current->prev->next = film;
        ring->current->prev = film;
        ring->current = film;
    }
    ring->size++;
    return film;
}

// Удаление фильма из списка
void remove_film(Films *films, Film *film)
{
    if (films->current == film && film->next == film) {
        films->current = NULL;
    } else {
        film->prev->next = film->next;
        film->next->prev = film->prev;

        if (films->current == film) films->current = film->next;
    }
    films->size--;
    free(film);
}

// Считываем данные о фильмах из файла
Films *get_films_from_file(const char *filename) {
    FILE *films_txt = fopen(filename, "r");
    Films *films = create_films_ring();
    if (films_txt == NULL) {
        return films;
    }

    fseek(films_txt, 0, SEEK_END);
    long size = ftell(films_txt);
    if (0 == size) {
        return films;
    }
    fseek(films_txt, 0, SEEK_SET);

    while (!feof(films_txt)) {
        Film *film = add_film(films);
        fgets(film->title, sizeof(film->title), films_txt);
        fscanf(films_txt, "%d\n", &film->year);
        fgets(film->countries, sizeof(film->countries), films_txt);
        fgets(film->genres, sizeof(film->genres), films_txt);
        fscanf(films_txt, "%f\n", &film->rating);

        strtok(film->title, "\n");
        strtok(film->countries, "\n");
        strtok(film->genres, "\n");
    }
    fclose(films_txt);
    return films;
}

// Вывод полного двусвязного списка для наглядности того, что все действительно работает
void print_circle(Films *films) {
    Film *film = films->current;
    do {
        printf("%s\n%d\n%s\n%s\n%.1f\n", film->title, film->year, film->countries, film->genres, film->rating);
        film = film->next;
    }
    while (films->current != film);
}

// Счетчик длины
size_t get_len(const char *string) {
    size_t size = 0;
    char byte;
    for (int i = 0;; i++) {
        byte = string[i];
        if (byte == '\0') {
            return size;
        }
        size += (((byte & 0x80) == 0) || ((byte & 0xC0) == 0xC0));
    }
}

// Вывод линий с информацией о фильме.
void print_line_with_spaces(const char *string, const char position) {
    size_t title_length = get_len(string), title_spaces;
    if (!position) {
        title_spaces = (46 - title_length) / 2;
    } else {
        title_length /= 2;
        if (title_length % 2 == 0) title_length++;
        title_spaces = (23 - title_length) / 2;
    }

    if (position != 2) printf("║");
    for (int j = 0; j < title_spaces; j++) printf(" ");

    text_red(stdout);
    if (!position) printf("%s", string);
    else for (int j = 0; j < title_length; j++) printf("*");
    text_blue(stdout);

    if ((!position && title_length % 2 != 0) || (position && title_length % 2 == 0)) title_spaces++;
    for (int j = 0; j < title_spaces; j++) printf(" ");

    if (position != 1) printf("║");

}

// Вывод карусели с фильмами
void print_cards(Film *film) {
    text_bold(stdout);
    text_blue(stdout);
    printf("                        ╔══════════════════════════════════════════════╗");
    printf("                        \n                        ");
    printf("║                                              ║");
    printf("                        \n                        ");
    printf("║                                              ║");
    printf("                        \n╔═══════════════════════");
    print_line_with_spaces(film->title, 0);
    printf("═══════════════════════╗\n║                       ");
    printf("║                                              ║");
    printf("                       ║\n");
    print_line_with_spaces(film->prev->title, 1);
    print_line_with_spaces(film->genres, 0);
    print_line_with_spaces(film->next->title, 2);
    printf("\n║                       ");
    printf("║                                              ║");
    printf("                       ║\n");
    print_line_with_spaces(film->prev->genres, 1);
    print_line_with_spaces(film->countries, 0);
    print_line_with_spaces(film->next->genres, 2);
    printf("\n║                       ");
    printf("║                                              ║");
    printf("                       ║\n");
    print_line_with_spaces(film->prev->countries, 1);
    printf("║                     %.1f+                     ║", film->rating);
    print_line_with_spaces(film->next->countries, 2);
    printf("\n║                       ");
    printf("║                                              ║");
    printf("                       ║\n║                       ");
    printf("║                                              ║");
    printf("                       ║\n║                       ");
    printf("║                                              ║");
    printf("                       ║\n║          ***          ");
    printf("║                                              ║");
    printf("          ***          ║\n║                       ");
    printf("║                                              ║");
    printf("                       ║\n╚═══════════════════════");
    printf("║                     %d                     ║", film->year);
    printf("═══════════════════════╝\n                        ");
    printf("║                                              ║");
    printf("                        \n                        ");
    printf("║                                              ║");
    printf("                        \n                        ");
    printf("╚══════════════════════════════════════════════╝\n");
    reset_colors(stdout);
}

// Вывод гигачада
void print_gigachad() {
    printf("            Пока-пока!\n");
    text_bold(stdout);
    text_green(stdout);
    printf("╔══════════════════════════════╗\n");
    printf("║⣿⣿⣿⣿⣿⣿⣿⣿⡿⠿⠛⠛⠛⠋⠉⠈⠉⠉⠉⠉⠛⠻⢿⣿⣿⣿⣿⣿⣿⣿║\n");
    printf("║⣿⣿⣿⣿⣿⡿⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⠛⢿⣿⣿⣿⣿║\n");
    printf("║⣿⣿⣿⣿⡏⣀⠀⠀⠀⠀⠀⠀⠀⣀⣤⣤⣤⣄⡀⠀⠀⠀⠀⠀⠀⠀⠙⢿⣿⣿║\n");
    printf("║⣿⣿⣿⢏⣴⣿⣷⠀⠀⠀⠀⠀⢾⣿⣿⣿⣿⣿⣿⡆⠀⠀⠀⠀⠀⠀⠀⠈⣿⣿║\n");
    printf("║⣿⣿⣟⣾⣿⡟⠁⠀⠀⠀⠀⠀⢀⣾⣿⣿⣿⣿⣿⣷⢢⠀⠀⠀⠀⠀⠀⠀⢸⣿║\n");
    printf("║⣿⣿⣿⣿⣟⠀⡴⠄⠀⠀⠀⠀⠀⠀⠙⠻⣿⣿⣿⣿⣷⣄⠀⠀⠀⠀⠀⠀⠀⣿║\n");
    printf("║⣿⣿⣿⠟⠻⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠶⢴⣿⣿⣿⣿⣿⣧⠀⠀⠀⠀⠀⠀⣿║\n");
    printf("║⣿⣁⡀⠀⠀⢰⢠⣦⠀⠀⠀⠀⠀⠀⠀⠀⢀⣼⣿⣿⣿⣿⣿⡄⠀⣴⣶⣿⡄⣿║\n");
    printf("║⣿⡋⠀⠀⠀⠎⢸⣿⡆⠀⠀⠀⠀⠀⠀⣴⣿⣿⣿⣿⣿⣿⣿⠗⢘⣿⣟⠛⠿⣼║\n");
    printf("║⣿⣿⠋⢀⡌⢰⣿⡿⢿⡀⠀⠀⠀⠀⠀⠙⠿⣿⣿⣿⣿⣿⡇⠀⢸⣿⣿⣧⢀⣼║\n");
    printf("║⣿⣿⣷⢻⠄⠘⠛⠋⠛⠃⠀⠀⠀⠀⠀⢿⣧⠈⠉⠙⠛⠋⠀⠀⠀⣿⣿⣿⣿⣿║\n");
    printf("║⣿⣿⣧⠀⠈⢸⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠟⠀⠀⠀⠀⢀⢃⠀⠀⢸⣿⣿⣿⣿║\n");
    printf("║⣿⣿⡿⠀⠴⢗⣠⣤⣴⡶⠶⠖⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⡸⠀⣿⣿⣿⣿║\n");
    printf("║⣿⣿⣿⡀⢠⣾⣿⠏⠀⠠⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠛⠉⠀⣿⣿⣿⣿║\n");
    printf("║⣿⣿⣿⣧⠈⢹⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣰⣿⣿⣿⣿║\n");
    printf("║⣿⣿⣿⣿⣧⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣠⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿║\n");
    printf("║⣿⣿⣿⣿⣷⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣴⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿║\n");
    printf("║⣿⣿⣿⣿⣿⣦⣄⣀⣀⣀⣀⠀⠀⠀⠀⠘⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿║\n");
    printf("║⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⡄⠀⠀⠀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿║\n");
    printf("║⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣧⠀⠀⠀⠙⣿⣿⡟⢻⣿⣿⣿⣿⣿⣿⣿⣿⣿║\n");
    printf("║⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠇⠀⠁⠀⠀⠹⣿⠃⠀⣿⣿⣿⣿⣿⣿⣿⣿⣿║\n");
    printf("║⣿⣿⣿⣿⣿⣿⣿⣿⡿⠛⣿⣿⠀⠀⠀⠀⠀⠀⠀⠀⢐⣿⣿⣿⣿⣿⣿⣿⣿⣿║\n");
    printf("║⣿⣿⣿⣿⠿⠛⠉⠉⠁⠀⢻⣿⡇⠀⠀⠀⠀⠀⠀⢀⠈⣿⣿⡿⠉⠛⠛⠛⠉⠉║\n");
    printf("║⣿⡿⠋⠁⠀⠀⢀⣀⣠⡴⣸⣿⣇⡄⠀⠀⠀⠀⢀⡿⠄⠙⠛⠀⣀⣠⣤⣤⠄⠄║\n");
    printf("╚══════════════════════════════╝\n");
    reset_colors(stdout);
}

// Поиск пользователя в базе данных из файла users.txt
User *get_user() {
    FILE *users_txt = fopen("users.txt", "r");
    User *user = (User*)malloc(sizeof(User));
    char user_input[24];

    regex_t regex;
    if (regcomp(&regex, "^[[:alpha:][:digit:]]{3,20}$", REG_EXTENDED)) {
        fprintf(stderr, "Не удалось скомпилировать regex-выражение.\n");
        exit(1);
    }

    while (1) {
        printf("Для входа или регистрации введите свой логин (от 3 до 20 символов) >> ");
        scanf("%s", user_input);

        if (regexec(&regex, user_input, 0, NULL, 0) == REG_NOMATCH) {
            printf("Логин должен состоять из латинских букв и цифр. И его длина должна быть в диапазоне 3-20 символов.\n");
            continue;
        }

        while (fgets(user->login, sizeof(user->login), users_txt)) {
            fgets(user->password, sizeof(user->password), users_txt);
            fgets(user->card, sizeof(user->card), users_txt);
            fscanf(users_txt, "%d\n", &user->is_admin);

            strtok(user->login, "\n");
            strtok(user->password, "\n");
            strtok(user->card, "\n");

            if (strstr(user->login, user_input)) {
                fclose(users_txt);
                return user;
            }
        }
        free(user);
        user = (User*)malloc(sizeof(User));
        strcpy(user->login, user_input);
        return user;
    }
}

// Авторизация существующего пользователя
char login(User* user) {
    char password[20];
    printf("Вход в аккаунт %s.\nВведите пароль >> ", user->login);
    scanf("%s", password);
    if (strstr(user->password, password)) {
        char filename[36] = "favorites/";
        strcat(filename, user->login);
        strcat(filename, ".txt");
        user->favorites = get_films_from_file(filename);

        return 1;
    }
    return 0;
}

// Сохранение данных о пользователе в файл
void save_user(User *user) {
    FILE *users_txt = fopen("users.txt", "a");
    fprintf(users_txt, "%s\n", user->login);
    fprintf(users_txt, "%s\n", user->password);
    fprintf(users_txt, "%s\n", user->card);
    fprintf(users_txt, "%d\n", user->is_admin);
    fclose(users_txt);
}

// Создание нового аккаунта для пользователя
void sign_up(User* user) {
    regex_t regex;
    if (regcomp(&regex, "^[[:digit:]]{16}$", REG_EXTENDED)) {
        fprintf(stderr, "Не удалось скомпилировать regex-выражение.\n");
        exit(1);
    }

    printf("Регистрация аккаунта с логином %s.\n", user->login);
    while (1) {
        printf("Введите пароль для аккаунта (от 6 до 20 символов) >> ");
        scanf("%s", user->password);
        int has_lowercase = 0, has_uppercase = 0, has_number = 0;
        for (int i = 0; i < strlen(user->password); i++) {
            if (islower(user->password[i])) has_lowercase++;
            if (isupper(user->password[i])) has_uppercase++;
            if (isdigit(user->password[i])) has_number++;
        }
        if (has_lowercase && has_uppercase && has_number && strlen(user->password) > 6) break;
        else
            printf("Пароль должен содержать хотя бы одну латинскую букву в верхнем и нижнем регистре. "
                   "И его длина должна быть в диапазоне от 3 до 20 символов\n");;
    }
    printf("Ваш будущий пароль для входа в аккаунта %s: %s\n", user->login, user->password);

    while (1) {
        printf("Введите номер карты без пробелов >> ");
        scanf("%s", user->card);
        if (regexec(&regex, user->card, 0, NULL, 0) == REG_NOMATCH)
            printf("Номер карты не должен содержать лишних символов и содержать только цифры. Длина должна составлять 16 цифр.\n");
        else break;
    }

    char filename[36] = "favorites/";
    strcat(filename, user->login);
    strcat(filename, ".txt");
    FILE *file = fopen(filename, "w");
    fclose(file);

    user->favorites = create_films_ring();
    user->is_admin = 0;
    save_user(user);
}

// Авторизация/Регистрация аккаунта
User *auth() {
    printf("Приветствую тебя в онлайн-кинотеатре.\n");
    while (1) {
        User *user = get_user();
        if (!strlen(user->password)) {
            system("clear");
            sign_up(user);
        }
        system("clear");
        if (!login(user)) {
            printf("Введён неверный пароль. Попробуйте войти снова.\n");
            free(user);
            continue;
        }
        return user;
    }
}

void write_film(FILE *file, Film *film) {
    fprintf(file, "%s\n", film->title);
    fprintf(file, "%d\n", film->year);
    fprintf(file, "%s\n", film->countries);
    fprintf(file, "%s\n", film->genres);
    fprintf(file, "%.1f\n", film->rating);
}

Film *add_favorite_film(User *user, Film *film) {
    Film *favorite_film = add_film(user->favorites);
    strcpy(favorite_film->title, film->title);
    favorite_film->year = film->year;
    strcpy(favorite_film->countries, film->countries);
    strcpy(favorite_film->genres, film->genres);
    favorite_film->rating = film->rating;

    char filename[36] = "favorites/";
    strcat(filename, user->login);
    strcat(filename, ".txt");

    FILE *favorites_file = fopen(filename, "a");
    write_film(favorites_file, favorite_film);
    fclose(favorites_file);

    return favorite_film;
}

void remove_favorite_film(User *user, Film *film) {
    char filename[36] = "favorites/";
    strcat(filename, user->login);
    strcat(filename, ".txt");

    FILE *temp_file = fopen("temp.txt", "a");
    Film *favorite_film = user->favorites->current;
    do {
        if (strstr(favorite_film->title, film->title)) {
            remove_film(user->favorites, film);
            break;
        }
        favorite_film = favorite_film->next;
    } while (favorite_film != user->favorites->current);

    if (user->favorites->current) {
        favorite_film = user->favorites->current;
        do {
            write_film(temp_file, favorite_film);
            favorite_film = favorite_film->next;
        } while (favorite_film != user->favorites->current);
    }
    fclose(temp_file);
    rename("temp.txt", filename);
}

// Вывод подробной информации о фильме
void print_addition_info(User* user, Film *film, Film *in_favorites) {
    while (1) {
        system("clear");
        printf("Название: %s\n", film->title);
        printf("Год производства: %d\n", film->year);
        printf("Страны производства: %s\n", film->countries);
        printf("Жанры: %s\n", film->genres);
        printf("Рейтинг: %.1f\n\n", film->rating);
        printf("Q - выход, ");
        if (!in_favorites) printf("F - добавить в избранное.\n");
        else printf("R - удалить из избранных.\n");

        system("/bin/stty raw");
        int ch = getchar();
        system("/bin/stty cooked");
        if (ch == 'f' && !in_favorites) {
            in_favorites = add_favorite_film(user, film);
        } else if (ch == 'r' && in_favorites) {
            remove_favorite_film(user, in_favorites);
            in_favorites = NULL;
        } else if (ch == 'q') {
            return;
        }
    }
}

// Проверка фильма на наличие в списке избранных
Film *check_favorites(Films *favorites, Film *film) {
    Film *favorite_film = favorites->current;
    do {
        if (strstr(favorite_film->title, film->title)) return favorite_film;
        favorite_film = favorite_film->next;
    }
    while (favorites->current != favorite_film);

    return NULL;
}

// Навигация внутри карусели
char show_films(Films *films, User *user, char view_favorites) {
    Film *film = films->current;
    while (films->current != NULL) {
        system("clear");
        print_cards(film);
        printf("                             Переход между фильмами на кнопки A и D\n");
        printf("                      Подробная информация - M, ");

        Film *in_favorites = NULL;
        if (user->favorites->size) in_favorites = check_favorites(user->favorites, film);
        if (!in_favorites) printf("Добавить в избранные - F\n");
        else printf("Удалить из избранных - R\n");

        system("/bin/stty raw");
        int ch = getchar();
        system("/bin/stty cooked");

        if (ch == 'a') {
            film = film->prev;
        } else if (ch == 'd') {
            film = film->next;
        } else if (ch == 'f' && !in_favorites) {
            add_favorite_film(user, film);
        } else if (ch == 'r' && in_favorites){
            if (view_favorites) film = film->next;
            remove_favorite_film(user, in_favorites);
        } else if (ch == 'm') {
            print_addition_info(user, film, in_favorites);
        } else if (ch == 'q') {
            return 0;
        }
    }
    return 1;
}

// Меню навигации для пользователя
void navigation_menu(Films* films, User *user) {
    while (1) {
        printf("╔ Добро пожаловать в меню навигации. Перейдите в нужный вам раздел.\n");
        printf("╟ 1. Личный кабинет.\n");
        printf("╟ 2. Каталог всех фильмов.\n");
        printf("╟ 3. Каталог избранных фильмов.\n");
        if (user->is_admin) {
            printf("╟ 4. Админ-панель.\n");
            printf("╚ e. Выход.\n");
        } else {
            printf("╚ e. Выход.\n");
        }
        system("/bin/stty raw");
        int ch = getchar();
        system("/bin/stty cooked");

        char result = 0;
        if (ch == '1') {
            printf("Здесь будет личный кабинет.\n");
            // Переход в личный кабинет
        } else if (ch == '2') {
            result = show_films(films, user, 0);
        } else if (ch == '3') {
            result = show_films(user->favorites, user, 1);
        } else if (ch == '4' && user->is_admin) {
            printf("Здесь будет админ-панель.\n");
            // Переход в админ-панель
        } else if (ch == 'e') {
            return;
        }
        system("clear");
        if (ch == '2' && result) {
            printf("Пока что администратор не добавил ни одного фильма.\n");
        } else if (ch == '3' && result) {
            printf("У вас нет фильмов в списке избранных. Для начала добавьте их.\n");
        }
    }
}

int main() {
    setlocale(LC_ALL, "Russian");

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    if (w.ws_col < 96 || w.ws_row < 28) {
        printf("Минимальное разрешение терминала для успешной работы 96x28.\n");
        return 0;
    }

    Films *films = get_films_from_file("films.txt");
    system("clear");

    User *user = auth();
    system("clear");

    navigation_menu(films, user);
    system("clear");

    print_gigachad();
    return 0;
}