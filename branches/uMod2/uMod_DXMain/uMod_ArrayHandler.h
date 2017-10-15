/*
This file is part of Universal Modding Engine.


Universal Modding Engine is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Universal Modding Engine is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Universal Modding Engine.  If not, see <http://www.gnu.org/licenses/>.
*/



#ifndef uMod_FIELDHANDLER_H_
#define uMod_FIELDHANDLER_H_

#include "..\uMod_GlobalDefines.h"

/**
 * On entry hold the pointer to the fake texture data (file content) or the filename.
 *
 * An object is created for each texture to be modded from the server.
 */
class TextureFileContent
{
public:
  TextureFileContent();
  ~TextureFileContent();

  int AddRef(void) {return ++RefCounter;}
  int Release(void) {if (--RefCounter==0) {delete this; return 0;} else return RefCounter;}

  bool ForceReload; //!< to force a reload of the texture (only important, if it is already modded)
  wchar_t* pFileName; //!< store the texture file name (if available)
  char* pData; //!<  store texture file as file in memory (if available)
  unsigned int Size; //!< size of file (\a pData)
  int Reference; //!< for a fast delete in the FileHandler
  DWORD64 Hash; //!< hash value

private:
  int RefCounter;
};


/**
 * One object holds a pointer to the data (\a TextureFileContent)
 * and a Reference to all generated fake texture.
 *
 * An object is created for each texture to be modded from the server for each client separately.
 */
class TextureEntry
{
public:
  TextureEntry(void);
  ~TextureEntry();

  int AddTexture(IUnknown *tex);
  int RemoveTexture(IUnknown* tex);

  void SetContent(TextureFileContent *content);
  const TextureFileContent * Content() {return TexContent;}



  TextureFileContent *TexContent;
  int NumberOfTextures; //!< Number of textures loaded by the game, which match the hash and thus are switched
  IUnknown **Textures; //!< pointer to all texture loaded by the game, which match the hash and thus are switched;
};



/**
 * A simple vector class for pointers. The Object or struct must have a member variable \a int \a Reference.
 * This class stores the entries in chunks of memory. The address of each chunk is stored in \a Content.
 */
template <class T>
class uMod_TextureHandler
{
public:
  uMod_TextureHandler(void);
  ~uMod_TextureHandler(void);

  /**
   * Append an entry at the end of the vector
   * @param entry
   * @return RETURN_OK on success
   */
  int Add( T* entry);

  /**
   * Remove this entry from the vector
   * @param entry
   * @return RETURN_OK on success
   */
  int Remove( T* entry);

  /**
   * Returns the number of entries.
   * @return
   */
  int GetNumber(void) {return (Number);}

  /**
   * Return the pointer to the \a i the object
   * @param i index
   * @return
   */
  T *operator [] (int i) {if (i<0||i>=Number) return (NULL); else return (Content[i/FieldLength][i%FieldLength]);}

private:
  static const int FieldLength = 1024; //!< size of one chunk of memory
  long Number; //!< Number of entries.
  int FieldCounter; //!< Number of allocated chunks.
  T*** Content;//!< vector of pointers, which point to the chunks
};



typedef uMod_TextureHandler<TextureFileContent> uMod_FileHandler;










template <class T>
uMod_TextureHandler<T>::uMod_TextureHandler(void)
{
  Message("uMod_TextureHandler(void): %p\n", this);
  Number = 0;
  FieldCounter = 0;

  Content = NULL;
}

template <class T>
uMod_TextureHandler<T>::~uMod_TextureHandler(void)
{
  Message("~uMod_TextureHandler(void): %p\n", this);
  if (Content!=NULL)
  {
    for (int i=0; i<FieldCounter; i++) if (Content[i] != NULL) delete [] Content[i];
    delete [] Content;
  }
}

template <class T>
int uMod_TextureHandler<T>::Add(T* entry)
{
  //Message("uMod_TextureHandler::Add( %p): %p\n", entry, this);
  if (gl_ErrorState & uMod_ERROR_FATAL) return (RETURN_FATAL_ERROR);

  if (entry->Reference>=0) return (RETURN_TEXTURE_ALLREADY_ADDED);

  if (Number/FieldLength==FieldCounter)
  {
    T*** temp = NULL;
    try {temp = new T**[FieldCounter+10];}
    catch (...)
    {
      gl_ErrorState |= uMod_ERROR_MEMORY | uMod_ERROR_TEXTURE;
      return (RETURN_NO_MEMORY);
    }

    for (int i=0; i<FieldCounter; i++) temp[i] = Content[i];

    for (int i=FieldCounter; i<FieldCounter+10; i++) temp[i] = NULL;

    FieldCounter += 10;

    if (Content!=NULL) delete [] Content;

    Content = temp;
  }
  if (Number%FieldLength==0)
  {
    try {if (Content[Number/FieldLength]==NULL) Content[Number/FieldLength] = new T*[FieldLength];}
    catch (...)
    {
      Content[Number/FieldLength]=NULL;
      gl_ErrorState |= uMod_ERROR_MEMORY | uMod_ERROR_TEXTURE;
      return (RETURN_NO_MEMORY);
    }
  }

  Content[Number/FieldLength][Number%FieldLength] = entry;
  entry->Reference = Number++;

  return (RETURN_OK);
}


template <class T>
int uMod_TextureHandler<T>::Remove(T* entry) //will be called, if a texture is completely released
{
  //Message("uMod_TextureHandler::Remove( %p): %p\n", entry, this);
  if (gl_ErrorState & uMod_ERROR_FATAL) return (RETURN_FATAL_ERROR);

  int ref = entry->Reference;
  if (ref<0|| ref>=Number) return (RETURN_OK); // it is not in this list
  if (Content[ref/FieldLength][ref%FieldLength]!=entry) return (RETURN_OK); // it is not in this list

  if (ref<(--Number))
  {
    Content[ref/FieldLength][ref%FieldLength] = Content[Number/FieldLength][Number%FieldLength];
    Content[ref/FieldLength][ref%FieldLength]->Reference = ref;
  }
  entry->Reference = -1;
  return (RETURN_OK);
}






#endif /* uMod_FIELDHANDLER_H_ */
