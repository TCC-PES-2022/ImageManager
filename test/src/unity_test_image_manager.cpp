#include <gtest/gtest.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

#include <fstream>

#include "iimagemanager.h"

#define RELATIVE_IMAGE_DIR "/pes/images"
#define COMPATIBILITY_FILE "compatibility.xml"

#define CUSTOM_COMPATIBILITY_FILE "/tmp/compatibility.xml"
#define CUSTOM_COMPATIBILITY_CONTENT "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"             \
                                     "<COMPATIBILITY>\n"                                        \
                                     "    <SOFTWARE PN=\"00000001\">\n"                         \
                                     "        <LRU name=\"LRU_EX1_LEFT\" PN=\"EXEMPLO3\"/>\n"   \
                                     "        <LRU name=\"LRU_EX1_CENTER\" PN=\"EXEMPLO4\"/>\n" \
                                     "        <LRU name=\"LRU_EX1_RIGHT\" PN=\"EXEMPLO5\"/>\n"  \
                                     "    </SOFTWARE>\n"                                        \
                                     "</COMPATIBILITY>\n"

class ImageManagerTest : public ::testing::Test
{
protected:
    ImageManagerTest()
    {
    }

    ~ImageManagerTest() override
    {
    }

    void SetUp() override
    {
        ASSERT_EQ(IMAGE_OPERATION_OK, create_handler(&handler));
        imageDir = std::string(getenv("HOME")) + std::string(RELATIVE_IMAGE_DIR);
    }

    void TearDown() override
    {
        ASSERT_EQ(IMAGE_OPERATION_OK, destroy_handler(&handler));
    }

    ImageHandlerPtr handler;
    std::string imageDir;
};

TEST_F(ImageManagerTest, ImportImageBinaryTest)
{
    char *pn = NULL;
    ImageOperationResult result = import_image(handler, "origin_images/load1.bin", &pn);

    ASSERT_EQ(result, IMAGE_OPERATION_OK);
    ASSERT_NE(pn, nullptr);
    ASSERT_STREQ(pn, "00000001");

    // Check if file was imported
    struct dirent *de;
    DIR *dr = opendir(imageDir.c_str());

    if (dr == NULL)
    {
        FAIL() << "Could not open " << imageDir << " directory";
    }

    bool found = false;
    std::string fileName = std::string(pn) + ".bin";
    while ((de = readdir(dr)) != NULL)
    {
        if (de->d_type == DT_REG && strcmp(de->d_name, fileName.c_str()) == 0)
        {
            found = true;
            break;
        }
    }

    closedir(dr);
    ASSERT_TRUE(found);
}

TEST_F(ImageManagerTest, ImportMultipleImageBinaryTest)
{
    // TODO: Make it an array
    char *pn1 = NULL;
    char *pn2 = NULL;
    char *pn3 = NULL;

    ImageOperationResult result1 = import_image(handler, "origin_images/load1.bin", &pn1);
    ImageOperationResult result2 = import_image(handler, "origin_images/load2.bin", &pn2);
    ImageOperationResult result3 = import_image(handler, "origin_images/load3.bin", &pn3);

    ASSERT_EQ(result1, IMAGE_OPERATION_OK);
    ASSERT_NE(pn1, nullptr);
    ASSERT_STREQ(pn1, "00000001");

    ASSERT_EQ(result2, IMAGE_OPERATION_OK);
    ASSERT_NE(pn2, nullptr);
    ASSERT_STREQ(pn2, "00000002");

    ASSERT_EQ(result3, IMAGE_OPERATION_OK);
    ASSERT_NE(pn3, nullptr);
    ASSERT_STREQ(pn3, "00000003");

    // Check if file was imported
    struct dirent *de;
    DIR *dr = opendir(imageDir.c_str());

    if (dr == NULL)
    {
        FAIL() << "Could not open " << imageDir << " directory";
    }

    bool found1 = false;
    bool found2 = false;
    bool found3 = false;
    std::string fileName1 = std::string(pn1) + ".bin";
    std::string fileName2 = std::string(pn2) + ".bin";
    std::string fileName3 = std::string(pn3) + ".bin";
    while ((de = readdir(dr)) != NULL)
    {
        if (de->d_type == DT_REG)
        {
            if (strcmp(de->d_name, fileName1.c_str()) == 0)
            {
                found1 = true;
            }
            else if (strcmp(de->d_name, fileName2.c_str()) == 0)
            {
                found2 = true;
            }
            else if (strcmp(de->d_name, fileName3.c_str()) == 0)
            {
                found3 = true;
            }
        }
    }

    closedir(dr);
    ASSERT_TRUE(found1);
    ASSERT_TRUE(found2);
    ASSERT_TRUE(found3);
}

TEST_F(ImageManagerTest, ImportCorruptedImageBinaryTest)
{
    char *pn = NULL;
    ImageOperationResult result = import_image(handler, "origin_images/corrupted_load1.bin", &pn);

    ASSERT_EQ(result, IMAGE_OPERATION_ERROR);
    ASSERT_EQ(pn, nullptr);
}

TEST_F(ImageManagerTest, ImportCompatibilityFileTest)
{
    char *pn = NULL;
    ImageOperationResult result = import_image(handler, "origin_images/ARQ_Compatibilidade1.xml", &pn);

    ASSERT_EQ(result, IMAGE_OPERATION_OK);
    ASSERT_NE(pn, nullptr);
    ASSERT_STREQ(pn, "00000000");

    // Check if file was imported
    struct dirent *de;
    DIR *dr = opendir(imageDir.c_str());

    if (dr == NULL)
    {
        FAIL() << "Could not open " << imageDir << " directory";
    }

    bool found = false;
    std::string fileName = std::string(COMPATIBILITY_FILE);
    while ((de = readdir(dr)) != NULL)
    {
        if (de->d_type == DT_REG && strcmp(de->d_name, fileName.c_str()) == 0)
        {
            found = true;
            break;
        }
    }

    closedir(dr);
    ASSERT_TRUE(found);
}

TEST_F(ImageManagerTest, ImportMultipleCompatibilityFileTest)
{
    // TODO: Make it an array
    char *pn1 = NULL;
    char *pn2 = NULL;
    char *pn3 = NULL;

    ImageOperationResult result1 = import_image(handler, "origin_images/ARQ_Compatibilidade1.xml", &pn1);
    ImageOperationResult result2 = import_image(handler, "origin_images/ARQ_Compatibilidade2.xml", &pn2);
    ImageOperationResult result3 = import_image(handler, "origin_images/ARQ_Compatibilidade3.xml", &pn3);

    ASSERT_EQ(result1, IMAGE_OPERATION_OK);
    ASSERT_NE(pn1, nullptr);
    ASSERT_STREQ(pn1, "00000000");

    ASSERT_EQ(result2, IMAGE_OPERATION_OK);
    ASSERT_NE(pn2, nullptr);
    ASSERT_STREQ(pn2, "00000000");

    ASSERT_EQ(result3, IMAGE_OPERATION_OK);
    ASSERT_NE(pn3, nullptr);
    ASSERT_STREQ(pn3, "00000000");

    // Check if file was imported
    struct dirent *de;
    DIR *dr = opendir(imageDir.c_str());

    if (dr == NULL)
    {
        FAIL() << "Could not open " << imageDir << " directory";
    }

    bool found = false;
    std::string fileName = std::string(COMPATIBILITY_FILE);
    while ((de = readdir(dr)) != NULL)
    {
        if (de->d_type == DT_REG && strcmp(de->d_name, fileName.c_str()) == 0)
        {
            found = true;
            break;
        }
    }

    closedir(dr);
    ASSERT_TRUE(found);

    // TODO: Assert if file was correctly merged
}

TEST_F(ImageManagerTest, ImportCorruptedCompatibilityFileTest)
{
    char *pn = NULL;
    ImageOperationResult result = import_image(handler, "origin_images/corrupted_ARQ_Compatibilidade.xml", &pn);

    ASSERT_EQ(result, IMAGE_OPERATION_ERROR);
    ASSERT_EQ(pn, nullptr);
}

TEST_F(ImageManagerTest, RemovePnTest)
{
    EXPECT_EQ(import_image(handler, "origin_images/ARQ_Compatibilidade1.xml", NULL), IMAGE_OPERATION_OK);

    const char *pn = "00000001";
    ASSERT_EQ(remove_image(handler, pn), IMAGE_OPERATION_OK);

    // Check if file was removed
    struct dirent *de;
    DIR *dr = opendir(imageDir.c_str());

    if (dr == NULL)
    {
        FAIL() << "Could not open " << imageDir << " directory";
    }

    bool found = false;
    std::string fileName = std::string(pn) + std::string(".bin");
    while ((de = readdir(dr)) != NULL)
    {
        if (de->d_type == DT_REG && strcmp(de->d_name, fileName.c_str()) == 0)
        {
            found = true;
            break;
        }
    }

    closedir(dr);
    ASSERT_FALSE(found);
}

TEST_F(ImageManagerTest, GetImagesTest)
{
    char *pn1 = "00000001";
    char *pn2 = "00000002";
    char *pn3 = "00000003";

    ASSERT_EQ(import_image(handler, "origin_images/load1.bin", &pn1), IMAGE_OPERATION_OK);
    ASSERT_EQ(import_image(handler, "origin_images/load2.bin", &pn2), IMAGE_OPERATION_OK);
    ASSERT_EQ(import_image(handler, "origin_images/load3.bin", &pn3), IMAGE_OPERATION_OK);

    char **images = NULL;
    int images_size = 0;
    ASSERT_EQ(get_images(handler, &images, &images_size), IMAGE_OPERATION_OK);

    ASSERT_EQ(images_size, 3);

    // Images may be out of order, so we need to find them
    bool found1 = false;
    bool found2 = false;
    bool found3 = false;

    for (int i = 0; i < images_size; i++)
    {
        if (strcmp(images[i], pn1) == 0)
        {
            found1 = true;
        }
        else if (strcmp(images[i], pn2) == 0)
        {
            found2 = true;
        }
        else if (strcmp(images[i], pn3) == 0)
        {
            found3 = true;
        }
    }

    ASSERT_TRUE(found1);
    ASSERT_TRUE(found2);
    ASSERT_TRUE(found3);
}

TEST_F(ImageManagerTest, GetImagePathTest)
{
    char *pn = NULL;
    ASSERT_EQ(import_image(handler, "origin_images/load1.bin", &pn), IMAGE_OPERATION_OK);

    char *path = NULL;
    ASSERT_EQ(get_image_path(handler, pn, &path), IMAGE_OPERATION_OK);

    std::string expectedPath = imageDir + "/" + pn + ".bin";
    ASSERT_STREQ(path, expectedPath.c_str());
}

TEST_F(ImageManagerTest, GetCompatibilityPathTest)
{
    ASSERT_EQ(import_image(handler, "origin_images/load1.bin", NULL), IMAGE_OPERATION_OK);

    const char *pnlist[] = {"00000001"};

    char *path = NULL;
    ASSERT_EQ(get_compatibility_path(handler, pnlist, 1, &path), IMAGE_OPERATION_OK);
    ASSERT_STREQ(path, CUSTOM_COMPATIBILITY_FILE);

    //Assert content
    std::ifstream file(path);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    ASSERT_STREQ(content.c_str(), CUSTOM_COMPATIBILITY_CONTENT);
}
