/*
 * Copyright (C) 2012 Ilias Stamatis <stamatis.iliass@gmail.com>
 * 
 * This file is part of Hunt.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mylibrary.h"
#include "animals.h"
#include "weapons.h"

#define MAX_INPUT              255

#define STARTING_ANIMALS         5
#define ADD_ANIM_ROUNDS          5  // add animal after each x rounds

#define MAX_CMD_ALIASES          2
#define MAX_CMD_PARAMS           2

#define DRUG_HEALTH             10
#define DRUG_COST               30
#define BULLET_COST              5

typedef enum {
    CMD_INVALID = -1,
    CMD_SHOOT,
    CMD_LOOK,
    CMD_BUY,
    CMD_STATUS,
    CMD_HELP,
    CMD_EXIT,
    MAX_COMMANDS // their total count
} ComTypeId;

typedef struct {
    ComTypeId   id;
    char        *aliases[MAX_CMD_ALIASES];
    int         nparams;  // num of params
    char        *params[MAX_CMD_PARAMS];
} CommandType;


typedef struct {
    int         health; // 0 to 100
    int         xp;
    int         gold;
    int         bullets;
    WeaponType  weapon;
    AniTypeId   capedanimals[MAX_ANIMALTYPES]; // captured animals
} Player;


void player_status(Player player)
{
    printf("Health: %d, XP: %d, Gold: %d\n"
           "Weapon: %s (attack=%d, distance=%d, value=%d), Bullets: %d\n",
           player.health, player.xp, player.gold, player.weapon.name,
           player.weapon.attack, player.weapon.distance,
           player.weapon.value, player.bullets);
}

void exit_msg(Player player, AnimalType animtable[])
{
    printf("\nThis adventure is over.\nYou've made %d xp points and died "
           "holding a %s.\n", player.xp, player.weapon.name);
           
    for (register int y = 0; y < MAX_ANIMALTYPES; y++)
        if (player.capedanimals[y] > 0)
            goto print_capedanimals;
    puts("\nYou haven't captured any animals.\n");
    return;

print_capedanimals:    
    printf("\nYou have also captured: ");
    for (register int i = 0; i < MAX_ANIMALTYPES; i++)
        if (player.capedanimals[i] > 0)
            printf("%d %ss, ", player.capedanimals[i], animtable[i].name);
    puts("\n");
}

bool shoot(CommandType command, List *list, Player *player)
{    
    SceneAnimal *animal = NULL;    
    int id = strtol(command.params[0], NULL, 10), damage;
    
    if (command.params[command.nparams] != NULL) {
        puts("You can't shoot two animals at once.");
        return false;
    }
    
    if (id == 0 || (animal = animals_find(list->head, id)) == NULL) {
        printf("%s is not a valid animal id\n", command.params[0]);
        return false;
    }
    
    if (!player->bullets) {
        puts("I have no bullets! I can't shoot!");
        return false;
    }
    
    puts("BOOM!");
    player->bullets--;
    if (player->weapon.distance < animal->distance)
        return true;
    
    damage = player->weapon.attack + 50 - animal->type.defense;
    // a divergence 0 to 10 from the standard
    damage += ((rand() % 5) + 1) - ((rand() % 5) + 1);    
    animal->health -= damage;
    
    if (animal->health <= 0) {
        printf("The %s is dead! You earn %d gold!\n", 
            animal->type.name, animal->type.value);
        player->gold += animal->type.value;
        player->xp += animal->type.value;
        player->capedanimals[animal->type.id]++;
        list->len--;
        animals_kill(&list->head, id);        
    };
    return true;
}

void buy(CommandType command, Player *player)
{
    char input[MAX_INPUT+1], *tokens[2] = {NULL};
    int num, cost;
    
    if (command.params[0] == NULL) {
        puts("What do you want to buy?");
        return;
    }
    
    if (!strcmp(command.params[0], "bullets") || 
        !strcmp(command.params[0], "drugs")) {
        
        if (command.params[1] == NULL) {
            printf("How much %s do you want?\n", command.params[0]);
            return;
        }
        
        num = strtol(command.params[1], NULL, 10);
        if (num <= 0) {
            printf("%s is not a valid positive integer.\n", command.params[1]);
            return;
        }
        
        cost = !strcmp(command.params[0], "bullets") ? BULLET_COST : DRUG_COST;
        cost *= num;        
        printf("You need %d gold in order to buy this stuff.\n", cost);        
        if (player->gold < cost) {
            puts("Sorry, you don't have enough gold.");
            return;
        }
        
        printf("Are you sure that you want to proceed? (y/n) ");
        fgets(input, sizeof(input), stdin);        
        s_tokenize(input, tokens, 2, " \n");
        
        if (tokens[0] == NULL || tokens[1] != NULL || 
                (strcmp(s_tolower(tokens[0]), "y") && 
                strcmp(s_tolower(tokens[0]), "yes"))) {
            puts("canceled");
            return;
            }            
        
        if (!strcmp(command.params[0], "bullets")) {
            player->gold -= cost;
            player->bullets += num;            
        }
        
        else {
            player->gold -= cost;
            player->health += num * DRUG_HEALTH;
            if (player->health > 100)
                player->health = 100;
        }
        
        puts("Done!");
        return;
    }
    
    if (! strcmp(command.params[0], "weapon")) {
        puts("buy weapon");
        return;
    }        
    
    printf("You can't buy %s.\n", command.params[0]);
} 


ComTypeId lookup_alias(CommandType cmdtable[], char *alias)
{
    if (alias == NULL)
        return CMD_INVALID;

    for (register int i = 0; i < MAX_COMMANDS; i++) {
        for (register int y = 0; y < MAX_CMD_ALIASES; y++)
            if (cmdtable[i].aliases[y] != NULL &&
                !strcmp(cmdtable[i].aliases[y], alias))
                return cmdtable[i].id;
    }
    return CMD_INVALID;
}

void parser(char *input, CommandType *command, CommandType cmdtable[])
{
    ComTypeId ret;
    char *tokens[MAX_CMD_PARAMS+1] = {NULL};
    int ntokens = 0;

    s_tolower(input);
    ntokens = s_tokenize(input, tokens, MAX_CMD_PARAMS+1, " \n");

    ret = lookup_alias(cmdtable, tokens[0]);
    if (ret == CMD_INVALID) {
        command->id = CMD_INVALID;
        return;
    }

    memcpy(command, &cmdtable[ret], sizeof(CommandType));
    for (register int i = 0; i < (ntokens - 1); i++)
        command->params[i] = tokens[i+1];
}


int main(void)
{
    AnimalType animtable[MAX_ANIMALTYPES] = {
        // .id            .name      .attack  .defense .value
        {ANIM_LION,      "lion",       100,      90,    100},
        {ANIM_TIGER,     "tiger",       95,      80,     90},
        {ANIM_CHEETAH,   "cheetah",     95,      80,     90},
        {ANIM_WOLF,      "wolf",        90,      70,     80},
        {ANIM_BEAR,      "bear",        75,      75,     75},
        {ANIM_ELEPHANT,  "elephant",    70,      80,     75},
        {ANIM_BOAR,      "boar",        90,      55,     70},
        {ANIM_ALLIGATOR, "alligator",   65,      50,     60},
        {ANIM_PYTHON,    "python",      60,      55,     60},
        {ANIM_FOX,       "fox",         55,      65,     60},
        {ANIM_DEER,      "deer",        50,      70,     60},
        {ANIM_ZEBRA,     "zebra",       50,      65,     55}
    };

    WeaponType weaptable[MAX_WEAPONS] = {
        // .id             .name    .attack  .distance .value
        {WEAP_SHOTGUN,   "Shotgun",   100,      100,     200},
        {WEAP_RIFLE,     "Rifle",      85,       80,     100},
        {WEAP_HANDGUN,   "Handgun",    65,       50,      50},
        {WEAP_SLING,     "Sling",      50,       30,       0}
    };

    CommandType cmdtable[MAX_COMMANDS] = {
        // .id             .aliases     .nparams     .params
        {CMD_SHOOT,    {"shoot",  NULL},    1,    {NULL, NULL}},
        {CMD_LOOK,     {"look",   NULL},    0,    {NULL, NULL}},
        {CMD_BUY,      {"buy",    NULL},    2,    {NULL, NULL}},
        {CMD_STATUS,   {"status", NULL},    0,    {NULL, NULL}},
        {CMD_HELP,     {"help",    "?"},    0,    {NULL, NULL}},
        {CMD_EXIT,     {"exit", "quit"},    0,    {NULL, NULL}},
    };

               // .health   .xp  .gold .bullets .weapon  .capendanimals
    Player player = {100,    0,    100,    3,  weaptable[3],   {0}};
    List animals = {NULL, NULL, 0, 0};

    char input[MAX_INPUT+1];
    int rounds = 0;
    CommandType command = {CMD_INVALID, {NULL, NULL}, 0, {NULL, NULL}};

    srand((unsigned) time(NULL));

    for (register int i = 0; i < STARTING_ANIMALS; i++)
        animals_addanimal(&animals, animtable);

    for (;;) {
        printf(">>> ");
        fgets(input, sizeof(input), stdin);
        parser(input, &command, cmdtable);

        switch (command.id) {
            case CMD_SHOOT: 
                if (shoot(command, &animals, &player)) 
                    goto animals_turn; 
                break;
            case CMD_LOOK: 
                animals_look(animals); 
                goto animals_turn;
            case CMD_BUY: 
                buy(command, &player);
                break;
            case CMD_STATUS: 
                player_status(player); 
                break;
            case CMD_HELP: 
                puts("cmdhelp"); 
                break;
            case CMD_EXIT: 
                exit_msg(player, animtable); 
                goto exit_success;
            case CMD_INVALID: default: 
                puts("Unkown command"); 
                break;
        }
        continue;

    animals_turn:        
        rounds++;
        if (!(rounds % ADD_ANIM_ROUNDS))
            if (animals_addanimal(&animals, animtable))
                puts("\nBe careful! Α new animal appeared from nowhere!");
    }

exit_success:
    animals_killall(animals.head);
    exit(EXIT_SUCCESS);
}
