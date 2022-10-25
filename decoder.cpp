#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <vector>

struct Info {
  uint8_t *name;
  uint8_t *description;
  uint8_t *maker;
  bool isPublic;
  uint32_t count;
  int64_t lastModified;
};

struct Entry
{
  uint8_t type;
  uint8_t *data;
  uint16_t length;
};

uint8_t* readShortString(uint8_t *data, uint8_t length) {
  uint8_t *text = (uint8_t *) calloc(256, 1);
  uint8_t *pointer = text;
  for (uint8_t i = 0; i < length; i++)
  {
    text[i] = *data;
    data += 1;
  };
  return pointer;
}

uint8_t* readString(uint8_t **data, uint16_t length) {
  uint8_t *text = (uint8_t *) calloc(length, 1);
  uint8_t *pointer = text;
  for (uint8_t i = 0; i < length; i++)
  {
    uint8_t copy = **data;
    text[i] = copy;
    *data += 1;
  };
  text[length] = 0;
  return pointer;
}

uint8_t readByte(uint8_t **data) {
  uint8_t byte = **data;
  *data += 1;
  return byte;
}

bool checkStartingHeader(uint8_t *data) {
  const char format[8] = {'t','o','6','o',0,'v','1',0};
  for (uint8_t i = 0; i < 8; i++)
  {
    if (data[i] != format[i]) return false;
  }
  return true;
}

bool isInfoHeader(uint8_t **data) {
  const uint8_t format[4] = {'I','N','F','O'};
  uint8_t *point = *data;
  for (uint8_t i = 0; i < 4; i++)
  {
    uint8_t testChar = format[i];
    uint8_t yes = point[i];

    if (yes != testChar) 
    {
      *data += 4;
      return false;
    }
  }
  *data += 4;
  return true;
}

bool isPlayHeader(uint8_t **data) {
  const uint8_t format[4] = {'P','L','A','Y'};
  uint8_t *point = *data;
  for (uint8_t i = 0; i < 4; i++)
  {
    uint8_t testChar = format[i];
    uint8_t yes = point[i];

    if (yes != testChar) 
    {
      *data += 4;
      return false;
    }
  }
  *data += 4;
  return true;
}

void copyShortString(uint8_t **data, uint8_t **dest) {
  uint8_t length = readByte(data);
  uint8_t *str = readShortString(*data, length);
  uint8_t *ret = (uint8_t *) calloc(256,1);
  for (size_t i = 0; i < length; i++)
  {
    ret[i] = str[i];
    *data += 1;
  }
  
  free(str);
  *dest = ret;
  return;
}

struct Info getInfo(uint8_t **data) {
  struct Info info;
  uint8_t features = **data << 4;
  *data += 1;
  
  bool hasMaker = ((features & 1) == 0), 
  hasName = ((features & 2) == 0), 
  hasDescription = ((features & 4) == 0),  
  isCountInt = ((features & 16) == 0);
  info.isPublic = ((features & 8) == 0);
  
  if (hasMaker) copyShortString(data, &info.maker);
  if (hasName) copyShortString(data, &info.name);
  if (hasDescription) copyShortString(data, &info.description);
  if (isCountInt) {
    uint32_t integer = **((uint32_t **) data);
    info.count = integer;
    *data += sizeof(uint32_t);
  }
  else {
    uint16_t shortInt = **((uint16_t **) data);
    info.count = shortInt; // Боли ме сърцето.
    *data += sizeof(uint16_t);
  }

  info.lastModified = **((int64_t **) data); 
  *data += sizeof(int64_t);
  return info;
}

uint32_t getEntries(uint8_t **data, uint8_t *start, uint32_t length, std::vector<Entry> *entries) {
  uint32_t pos = 0;
  uint32_t count = 2;
  while (*data - start < length)
  {
    uint16_t len = **((uint16_t **) data);
    *data += sizeof(uint16_t);

    struct Entry *entry = (struct Entry *) calloc(1, sizeof(struct Entry));

    (*entry).type = readByte(data);
    (*entry).data = readString(data, len);
    (*entry).length = len;

    (*entries).push_back(*entry);
  }
  return pos;
}

void decode(uint8_t *buf, uint32_t length, struct Info **information, std::vector<struct Entry> *entryPointer) {
  uint8_t *startingPosition = buf;
  uint32_t *entryLength = (uint32_t *) malloc(sizeof(uint32_t));
  if (!checkStartingHeader(buf)) return;
  uint8_t **bufPoint = &buf;
  *bufPoint += 8;
  struct Info *info = (struct Info *) malloc(sizeof(struct Info));

  if (isInfoHeader(bufPoint)) {
    *info = getInfo(bufPoint);
  }
  
  if (isPlayHeader(bufPoint)) {
    *entryLength = getEntries(bufPoint, startingPosition, length, entryPointer);
  }
  *information = info;
}

int main() {
  uint64_t length;
  uint8_t *buffer;
  std::vector<struct Entry> entries;
  std::vector<struct Entry> *entryPointer = &entries;
  struct Info in = {0,0,0,0,0,0};

  struct Info *info = &in;
  FILE *fp = fopen("./e30e0c85-e8c2-4d7c-848c-55ef32cca153.play", "rb");
  if (!fp) exit(1);

  fseek(fp, 0, SEEK_END);
  length = ftell(fp);
  rewind(fp);

  buffer = (uint8_t *) malloc(length);

  if (fread(buffer, length, 1, fp) != 1) {
    fputs("Copying file failed.\n", stderr);
    exit(1);
  }
  decode(buffer, length, &info, &entries);

  printf("Info: '%s' '%s' '%s'\n", info->name, info->maker, info->description);

  for (uint32_t i = 0; i < entries.size(); i++)
  {
    printf("[%u] - (%u): %s\n", i, entries[i].type, entries[i].data);
  }

  fclose(fp);
}
