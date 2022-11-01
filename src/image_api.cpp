#include <stdio.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include <string>
#include <unordered_map>
#include <iostream>

#include "iimagemanager.h"
#include "tinyxml2.h"
#include "gcrypt.h"

#define RELATIVE_IMAGE_DIR "/pes/images"

#define COMPATIBILITY_FILE_PN "00000000"
#define COMPATIBILITY_FILE "compatibility.xml"

#define CUSTOM_COMPATIBILITY_FILE "/tmp/compatibility.xml"

#define PN_SIZE 4
#define SHA256_SIZE 32

struct ImageHandler
{
    std::string imageDir;
    std::unordered_map<std::string, std::string> image_map;
    char **images = NULL;
    int get_list_size = 0;
};

static struct ImageHandler singletonHandler;

ImageOperationResult check_xml_file(const char *path, bool *isXMLFile)
{
    if (path == NULL || isXMLFile == NULL)
    {
        return IMAGE_OPERATION_ERROR;
    }

    tinyxml2::XMLDocument doc;
    *isXMLFile = (doc.LoadFile(path) == tinyxml2::XML_SUCCESS);

    return IMAGE_OPERATION_OK;
}

ImageOperationResult check_checksum(const char *path, bool *isValidChecksum)
{
    if (path == NULL || isValidChecksum == NULL)
    {
        return IMAGE_OPERATION_ERROR;
    }

    bool isXMLFile = false;
    if (check_xml_file(path, &isXMLFile) == IMAGE_OPERATION_OK && !isXMLFile)
    {
        FILE *fp = fopen(path, "rb");
        if (fp == NULL)
        {
            return IMAGE_OPERATION_ERROR;
        }

        fseek(fp, 0, SEEK_END);
        if (ftell(fp) < (PN_SIZE + SHA256_SIZE))
        {
            fclose(fp);
            return IMAGE_OPERATION_ERROR;
        }

        fseek(fp, PN_SIZE, SEEK_SET);

        // Read SHA256
        char filesha256[SHA256_SIZE + 1];
        fread(filesha256, 1, SHA256_SIZE, fp);
        filesha256[SHA256_SIZE] = '\0';

        // Read data
        fseek(fp, 0, SEEK_END);
        size_t dataSize = ftell(fp) - PN_SIZE - SHA256_SIZE;
        fseek(fp, PN_SIZE + SHA256_SIZE, SEEK_SET);
        char *data = new char[dataSize];
        fread(data, 1, dataSize, fp);

        gcry_md_hd_t hd;
        gcry_md_open(&hd, GCRY_MD_SHA256, 0);
        gcry_md_write(hd, data, dataSize);
        unsigned char *digest = gcry_md_read(hd, GCRY_MD_SHA256);

        *isValidChecksum = (memcmp(filesha256, digest, SHA256_SIZE) == 0);

        delete[] data;
        fclose(fp);
    }
    return IMAGE_OPERATION_OK;
}

ImageOperationResult create_handler(ImageHandlerPtr *handler)
{
    if (handler == NULL)
    {
        return IMAGE_OPERATION_ERROR;
    }

    *handler = &singletonHandler;

    singletonHandler.imageDir = std::string(getenv("HOME")) + std::string(RELATIVE_IMAGE_DIR);

    // Create image directory if it does not exist
    struct stat buffer;
    if (stat(singletonHandler.imageDir.c_str(), &buffer) != 0)
    {
        if (mkdir(singletonHandler.imageDir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) != 0)
        {
            printf("[ERROR] Could create %s directory", singletonHandler.imageDir.c_str());
            return IMAGE_OPERATION_ERROR;
        }
    }

    // TODO: Make sure directory has correct permissions

    // Load image list from disk
    struct dirent *de;
    DIR *dr = opendir(singletonHandler.imageDir.c_str());

    if (dr == NULL)
    {
        printf("[ERROR] Could not open %s directory", singletonHandler.imageDir.c_str());
        return IMAGE_OPERATION_ERROR;
    }

    while ((de = readdir(dr)) != NULL)
    {
        if (de->d_type == DT_REG)
        {
            std::string fileName = de->d_name;
            std::string baseName = fileName.substr(0, fileName.find_last_of("."));
            std::string filePath = singletonHandler.imageDir + "/" + fileName;

            bool isValidChecksum = false;
            if (check_checksum(filePath.c_str(), &isValidChecksum) == IMAGE_OPERATION_OK && isValidChecksum == true)
            {
                singletonHandler.image_map[baseName] = filePath;
            }
        }
    }

    closedir(dr);
    return IMAGE_OPERATION_OK;
}

ImageOperationResult destroy_handler(ImageHandlerPtr *handler)
{
    if (handler == NULL)
    {
        return IMAGE_OPERATION_ERROR;
    }

    if (singletonHandler.images != NULL)
    {
        for (int i = 0; i < singletonHandler.get_list_size; i++)
        {
            free(singletonHandler.images[i]);
        }

        free(singletonHandler.images);
        singletonHandler.images = NULL;
    }

    *handler = NULL;
    return IMAGE_OPERATION_OK;
}

ImageOperationResult import_image(ImageHandlerPtr handler, const char *path, char **part_number)
{
    if (handler == NULL || path == NULL)
    {
        return IMAGE_OPERATION_ERROR;
    }

    bool isXMLFile = false;
    if (check_xml_file(path, &isXMLFile) != IMAGE_OPERATION_OK)
    {
        return IMAGE_OPERATION_ERROR;
    }

    if (isXMLFile == true)
    {
        FILE *fpOrig = fopen(path, "r");
        std::string destFile = singletonHandler.imageDir + std::string("/") + std::string(COMPATIBILITY_FILE);
        FILE *fpDest = fopen(destFile.c_str(), "a+");
        fseek(fpDest, 0, SEEK_END);
        bool firstTime = (ftell(fpDest) == 0);
        fseek(fpDest, 0, SEEK_SET);
        if (fpOrig == NULL || fpDest == NULL)
        {
            return IMAGE_OPERATION_ERROR;
        }

        if (firstTime)
        {
            // TODO: Copy file needs to be a function
            fseek(fpOrig, 0, SEEK_SET);
            char buffer[1024];
            size_t bytesRead = 0;
            while ((bytesRead = fread(buffer, 1, 1024, fpOrig)) > 0)
            {
                fwrite(buffer, 1, bytesRead, fpDest);
            }
        }
        else
        {
            // If it's not the first time, we need to update the existing file

            tinyxml2::XMLDocument docOrig;
            tinyxml2::XMLDocument docDest;
            if (docOrig.LoadFile(fpOrig) != tinyxml2::XML_SUCCESS || docDest.LoadFile(fpDest) != tinyxml2::XML_SUCCESS)
            {
                fclose(fpOrig);
                fclose(fpDest);
                return IMAGE_OPERATION_ERROR;
            }

            tinyxml2::XMLElement *rootOrig = docOrig.RootElement();
            tinyxml2::XMLElement *rootDest = docDest.RootElement();
            if (rootOrig == NULL || rootDest == NULL)
            {
                fclose(fpOrig);
                fclose(fpDest);
                return IMAGE_OPERATION_ERROR;
            }

            tinyxml2::XMLElement *softElemOrig = rootOrig->FirstChildElement("SOFTWARE");

            if (softElemOrig == NULL)
            {
                fclose(fpOrig);
                fclose(fpDest);
                return IMAGE_OPERATION_ERROR;
            }

            bool error = false;
            for (; softElemOrig; softElemOrig = softElemOrig->NextSiblingElement())
            {
                const char *pnOrig = softElemOrig->Attribute("PN");
                if (pnOrig == NULL)
                {
                    error = true;
                    break;
                }

                tinyxml2::XMLElement *softElemDest = rootDest->FirstChildElement("SOFTWARE");
                if (softElemDest == NULL)
                {
                    error = true;
                    break;
                }

                // Check if part number already exists and replace it if it does
                for (; softElemDest; softElemDest = softElemDest->NextSiblingElement())
                {
                    const char *pnDest = softElemDest->Attribute("PN");
                    if (pnDest == NULL)
                    {
                        error = true;
                        break;
                    }

                    if (strcmp(pnOrig, pnDest) == 0)
                    {
                        // Delete current element to add a new one
                        rootDest->DeleteChild(softElemDest);
                        break;
                    }
                }

                // Add new element
                tinyxml2::XMLNode *newNode = softElemOrig->DeepClone(&docDest);
                rootDest->InsertEndChild(newNode);
                // tinyxml2::XMLPrinter printer(stdout);
                // docDest.Print(&printer);
            }

            if (error)
            {
                fclose(fpOrig);
                fclose(fpDest);
                return IMAGE_OPERATION_ERROR;
            }

            // If we save directly to the file, we'll duplicate the XML content.
            // Instead, we'll save to a temporary file and then copy it over the original file.
            //
            // TODO: There's probably a better way to do this
            std::string tmpDest = std::string("/tmp/") + std::string(COMPATIBILITY_FILE);
            docDest.SaveFile(tmpDest.c_str());

            // Now copy the temporary file over the original file - TODO: this needs to be a function !
            fclose(fpDest);
            fpDest = fopen(destFile.c_str(), "w");
            if (fpDest == NULL)
            {
                fclose(fpOrig);
                return IMAGE_OPERATION_ERROR;
            }

            FILE *fpTmp = fopen(tmpDest.c_str(), "r");
            if (fpTmp == NULL)
            {
                fclose(fpOrig);
                fclose(fpDest);
                return IMAGE_OPERATION_ERROR;
            }

            fseek(fpTmp, 0, SEEK_SET);
            char buffer[1024];
            size_t bytesRead = 0;
            while ((bytesRead = fread(buffer, 1, 1024, fpTmp)) > 0)
            {
                fwrite(buffer, 1, bytesRead, fpDest);
            }

            fclose(fpTmp);
        }

        fclose(fpOrig);
        fclose(fpDest);
        // handler->image_map[COMPATIBILITY_FILE_PN] = destFile;

        if (part_number != NULL)
        {
            *part_number = (char *)(COMPATIBILITY_FILE_PN);
        }
    }
    else
    {
        bool isValidChecksum = false;
        if (check_checksum(path, &isValidChecksum) != IMAGE_OPERATION_OK ||
            isValidChecksum == false)
        {
            return IMAGE_OPERATION_ERROR;
        }

        FILE *fpOrig = fopen(path, "rb");
        if (fpOrig == NULL)
        {
            printf("[ERROR] Could not open %s file", path);
            return IMAGE_OPERATION_ERROR;
        }

        fseek(fpOrig, 0, SEEK_SET);
        unsigned char pn[PN_SIZE];
        fread(pn, 1, PN_SIZE, fpOrig);

        std::string pnStr;
        for (int i = 0; i < PN_SIZE; i++)
        {
            // TO HEX STRING
            char hex[3];
            sprintf(hex, "%02X", pn[i]);
            pnStr = pnStr + hex;
        }

        std::string destPath = singletonHandler.imageDir + std::string("/") + pnStr + std::string(".bin");

        // Copy file
        fseek(fpOrig, 0, SEEK_SET);
        char buffer[1024];
        FILE *fpDest = fopen(destPath.c_str(), "wb");
        if (fpDest == NULL)
        {
            fclose(fpOrig);
            return IMAGE_OPERATION_ERROR;
        }

        size_t bytesRead = 0;
        while ((bytesRead = fread(buffer, 1, 1024, fpOrig)) > 0)
        {
            fwrite(buffer, 1, bytesRead, fpDest);
        }

        fclose(fpOrig);
        fclose(fpDest);

        isValidChecksum = false;
        if (check_checksum(destPath.c_str(), &isValidChecksum) != IMAGE_OPERATION_OK ||
            isValidChecksum == false)
        {
            unlink(destPath.c_str());
            return IMAGE_OPERATION_ERROR;
        }

        handler->image_map[pnStr] = destPath;

        if (part_number != NULL)
        {
            std::unordered_map<std::string, std::string>::iterator it = handler->image_map.find(pnStr);
            *part_number = (char *)(it->first.c_str());
        }
    }

    return IMAGE_OPERATION_OK;
}

ImageOperationResult remove_image(ImageHandlerPtr handler, const char *part_number)
{
    if (handler == NULL || part_number == NULL)
    {
        return IMAGE_OPERATION_ERROR;
    }

    // Nice try, but I won't let you remove the compatibility file for now.
    if (strcmp(part_number, COMPATIBILITY_FILE_PN) == 0)
    {
        return IMAGE_OPERATION_ERROR;
    }

    // for (std::unordered_map<std::string, std::string>::iterator it = handler->image_map.begin(); it != handler->image_map.end(); ++it)
    // {
    //     std::cout << it->first << " => " << it->second << '\n';
    // }

    if (unlink(handler->image_map[part_number].c_str()) != 0)
    {
        return IMAGE_OPERATION_ERROR;
    }

    handler->image_map.erase(part_number);

    // TODO: We could remove the PN from the compatibility file if it exists
    //       but it is not necessary because if this image is re-added it will
    //       override the existing entry in the compatibility file.

    return IMAGE_OPERATION_OK;
}

ImageOperationResult get_images(
    ImageHandlerPtr handler,
    char **part_numbers[],
    int *list_size)
{
    if (handler == NULL || list_size == NULL)
    {
        return IMAGE_OPERATION_ERROR;
    }

    if (handler->images != NULL)
    {
        for (int i = 0; i < handler->get_list_size; i++)
        {
            free(handler->images[i]);
        }

        free(handler->images);
        handler->images = NULL;
    }

    handler->get_list_size = handler->image_map.size();
    handler->images = (char **)malloc(sizeof(char *) * handler->get_list_size);
    int i = 0;
    for (std::unordered_map<std::string, std::string>::iterator it = handler->image_map.begin();
         it != handler->image_map.end(); ++it)
    {
        if (it->first.compare(COMPATIBILITY_FILE_PN) == 0)
        {
            continue;
        }

        handler->images[i++] = strdup(it->first.c_str());
    }

    *list_size = handler->get_list_size;
    *part_numbers = handler->images;
    return IMAGE_OPERATION_OK;
}

ImageOperationResult get_image_path(ImageHandlerPtr handler, const char *part_number, char **path)
{
    if (handler == NULL || part_number == NULL || path == NULL)
    {
        return IMAGE_OPERATION_ERROR;
    }

    std::string partNumberStr = std::string(part_number);
    std::unordered_map<std::string, std::string>::iterator it = handler->image_map.find(partNumberStr);
    if (it == handler->image_map.end())
    {
        *path = NULL;
        return IMAGE_OPERATION_ERROR;
    }

    *path = (char *)(it->second.c_str());

    return IMAGE_OPERATION_OK;
}

ImageOperationResult get_compatibility_path(
    ImageHandlerPtr handler,
    char **part_numbers,
    int list_size,
    char **path)
{
    if (handler == NULL || part_numbers == NULL || list_size == 0)
    {
        return IMAGE_OPERATION_ERROR;
    }

    // TODO: We'll copy all and remove the ones that don't match the PN list
    //       but we could also just copy the ones that match the PN list (and that looks better)

    // Copy compatibility file to temp directory
    std::string xmlPath = singletonHandler.imageDir + std::string("/") + COMPATIBILITY_FILE;
    FILE *fpOrig = fopen(xmlPath.c_str(), "rb");
    FILE *fpDest = fopen(CUSTOM_COMPATIBILITY_FILE, "wb");

    if (fpOrig == NULL || fpDest == NULL)
    {
        return IMAGE_OPERATION_ERROR;
    }

    fseek(fpOrig, 0, SEEK_SET);
    char buffer[1024];
    size_t bytesRead = 0;
    while ((bytesRead = fread(buffer, 1, 1024, fpOrig)) > 0)
    {
        fwrite(buffer, 1, bytesRead, fpDest);
    }

    fclose(fpOrig);
    fclose(fpDest);

    // Remove all PN that are not in the list
    tinyxml2::XMLDocument doc;
    if (doc.LoadFile(CUSTOM_COMPATIBILITY_FILE) != tinyxml2::XML_SUCCESS)
    {
        return IMAGE_OPERATION_ERROR;
    }

    tinyxml2::XMLElement *root = doc.RootElement();
    if (root == NULL)
    {
        return IMAGE_OPERATION_ERROR;
    }

    tinyxml2::XMLElement *softElem = root->FirstChildElement("SOFTWARE");
    if (softElem == NULL)
    {
        return IMAGE_OPERATION_ERROR;
    }

    bool error = false;
    while(softElem)
    {
        const char *pn = softElem->Attribute("PN");
        if (pn == NULL)
        {
            error = true;
            break;
        }

        bool found = false;
        for (int i = 0; i < list_size; i++)
        {
            if (strcmp(pn, part_numbers[i]) == 0)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            root->DeleteChild(softElem);
            softElem = root->FirstChildElement("SOFTWARE");
        }
        else
        {
            softElem = softElem->NextSiblingElement();
        }
    }

    if (error)
    {
        return IMAGE_OPERATION_ERROR;
    }

    doc.SaveFile(CUSTOM_COMPATIBILITY_FILE);
    *path = (char *)(CUSTOM_COMPATIBILITY_FILE);
    return IMAGE_OPERATION_OK;
}