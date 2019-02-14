#ifndef MD_CONFIG_H_
#define MD_CONFIG_H_
#pragma once

#ifdef READCONFIG_EXPORTS
#define READCONFIG_API __declspec(dllexport)
#else
#define READCONFIG_API __declspec(dllimport)
#endif READCONFIG_EXPORTS

READCONFIG_API void GetConfigString(const char *filename, char *key, char *val, int size);
READCONFIG_API int GetConfigInt(const char *filename, char *key);

#endif MD_CONFIG_H_
