#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

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
  uint8_t *text = (uint8_t *) malloc(256);
  uint8_t *pointer = text;
  for (uint8_t i = 0; i < length; i++)
  {
    text[i] = *data;
    data += 1;
  };
  return pointer;
}

uint8_t* readString(uint8_t **data, uint16_t length) {
  uint8_t *text = (uint8_t *) malloc(length);
  uint8_t *pointer = text;
  for (uint8_t i = 0; i < length; i++)
  {
    uint8_t copy = **data;
    text[i] = copy;
    *data += 1;
  };
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
  uint8_t *ret = (uint8_t *) malloc(256);
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
    uint16_t shorT = **((uint16_t **) data);
    info.count = shorT; // Боли ме сърцето.
    *data += sizeof(uint16_t);
  }

  info.lastModified = **((int64_t **) data); 
  *data += sizeof(int64_t);
  return info;
}

uint32_t getEntries(uint8_t **data, uint8_t *start, uint32_t length, struct Entry **entries) {
  uint32_t pos = 0;
  uint32_t count = 2;
  while (*data - start < length)
  {
    if (pos + 1 >= count) {
      uint32_t ncount = count * 2;
      struct Entry *p = *entries;
      *entries = (struct Entry *) malloc(ncount * sizeof(struct Entry));
      memcpy(p, *entries, count * sizeof(struct Entry));
      count = ncount;
      free(p);
    }

    uint16_t len = **((uint16_t **) data);
    *data += sizeof(uint16_t);

    (*entries)[pos].type = readByte(data);
    (*entries)[pos].data = readString(data, len);
    (*entries)[pos].length = len;
    pos += 1;
  }
  return pos;
}

uint32_t* decode(uint8_t *buf, uint32_t length, struct Info **information, struct Entry **items) {
  uint8_t *startingPosition = buf;
  uint32_t *entryLength = (uint32_t *) malloc(sizeof(uint32_t));
  if (!checkStartingHeader(buf)) return entryLength;
  uint8_t **bufPoint = &buf;
  *bufPoint += 8;
  struct Info *info = (struct Info *) malloc(sizeof(struct Info));

  if (isInfoHeader(bufPoint)) {
    *info = getInfo(bufPoint);
  }
  struct Entry *entries = (struct Entry *) malloc(sizeof(struct Entry));
  struct Entry **entryPointer = &entries;
  if (isPlayHeader(bufPoint)) {
    *entryLength = getEntries(bufPoint, startingPosition, length, entryPointer);
  }
  **items = *entries;
  *information = info;
  return entryLength;
}

int main() {
  uint64_t length;
  uint8_t *buffer;
  struct Entry en = {0,0,0};
  struct Info in = {0,0,0,0,0,0};

  struct Entry *items = &en;
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
  uint32_t decodedLength = *decode(buffer, length, &info, &items);

  for (uint32_t i = 0; i < decodedLength; i++)
  {
    struct Entry item = *items;
    printf("[%u] - (%u) \n", i, item.type);
    items += 1;
  }

  fclose(fp);
}
