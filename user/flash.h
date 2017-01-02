#ifndef FLASH_H
#define FLASH_H


void flash_erase_all();
int flash_key_value_set(const char *key,const char *value);
int flash_key_value_get(char *key,char *value);

#endif