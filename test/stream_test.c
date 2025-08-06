#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "data/stream/stream.h"
#include "log/log.h"

// 创建测试目录
int create_test_dir(const char *dir) {
    struct stat st = {0};
    if (stat(dir, &st) == -1) {
        if (mkdir(dir, 0755) == -1) {
            printf("Failed to create directory %s: %s\n", dir, strerror(errno));
            return -1;
        }
    }
    return 0;
}

// 测试配置解析
int test_stream_config_parsing() {
    printf("\n=== Testing Stream Config Parsing ===\n");
    
    // 测试正常配置
    const char *valid_json = "{"
        "\"format\": \"jpg\","
        "\"outputDir\": \"./test_output\","
        "\"frameCount\": 5,"
        "\"frameInterval\": 2000000000,"
        "\"videoNum\": 2"
    "}";
    
    StreamConfig config;
    int ret = stream_parse_config(valid_json, &config);
    if (ret != 0) {
        printf("❌ Failed to parse valid JSON config\n");
        return -1;
    }
    
    printf("✅ Successfully parsed config:\n");
    printf("   Format: %s\n", config.format);
    printf("   Output Dir: %s\n", config.outputDir);
    printf("   Frame Count: %d\n", config.frameCount);
    printf("   Frame Interval: %d ns\n", config.frameInterval);
    printf("   Video Num: %d\n", config.videoNum);
    
    // 验证解析结果
    if (strcmp(config.format, "jpg") != 0 ||
        strcmp(config.outputDir, "./test_output") != 0 ||
        config.frameCount != 5 ||
        config.frameInterval != 2000000000 ||
        config.videoNum != 2) {
        printf("❌ Config values don't match expected values\n");
        stream_free_config(&config);
        return -1;
    }
    
    stream_free_config(&config);
    
    // 测试默认值
    const char *empty_json = "{}";
    ret = stream_parse_config(empty_json, &config);
    if (ret != 0) {
        printf("❌ Failed to parse empty JSON config\n");
        return -1;
    }
    
    printf("✅ Default values:\n");
    printf("   Format: %s\n", config.format);
    printf("   Output Dir: %s\n", config.outputDir);
    printf("   Frame Count: %d\n", config.frameCount);
    printf("   Frame Interval: %d ns\n", config.frameInterval);
    printf("   Video Num: %d\n", config.videoNum);
    
    stream_free_config(&config);
    
    // 测试无效 JSON
    const char *invalid_json = "{invalid json}";
    ret = stream_parse_config(invalid_json, &config);
    if (ret == 0) {
        printf("❌ Should have failed to parse invalid JSON\n");
        stream_free_config(&config);
        return -1;
    }
    
    printf("✅ Correctly rejected invalid JSON\n");
    return 0;
}

// 测试文件名生成
int test_filename_generation() {
    printf("\n=== Testing Filename Generation ===\n");
    
    const char *test_dir = "./test_output";
    const char *format = "jpg";
    
    // 创建测试目录
    if (create_test_dir(test_dir) != 0) {
        return -1;
    }
    
    // 生成多个文件名测试唯一性
    char *filename1 = stream_gen_filename(test_dir, format);
    usleep(1000); // 等待1ms确保时间戳不同
    char *filename2 = stream_gen_filename(test_dir, format);
    
    if (!filename1 || !filename2) {
        printf("❌ Failed to generate filenames\n");
        return -1;
    }
    
    printf("✅ Generated filenames:\n");
    printf("   File 1: %s\n", filename1);
    printf("   File 2: %s\n", filename2);
    
    // 验证文件名不同
    if (strcmp(filename1, filename2) == 0) {
        printf("❌ Generated filenames are identical\n");
        free(filename1);
        free(filename2);
        return -1;
    }
    
    // 验证文件名格式
    if (strstr(filename1, test_dir) == NULL || strstr(filename1, format) == NULL) {
        printf("❌ Filename format is incorrect\n");
        free(filename1);
        free(filename2);
        return -1;
    }
    
    printf("✅ Filenames are unique and correctly formatted\n");
    
    free(filename1);
    free(filename2);
    return 0;
}

// 测试流处理（使用模拟数据）
int test_stream_processing_mock() {
    printf("\n=== Testing Stream Processing (Mock) ===\n");
    
    const char *test_dir = "./test_output";
    
    // 创建测试目录
    if (create_test_dir(test_dir) != 0) {
        return -1;
    }
    
    // 由于没有真实的 RTSP 流，我们测试错误处理
    const char *fake_url = "rtsp://fake.stream.url/test";
    
    printf("🔄 Testing error handling with fake RTSP URL...\n");
    
    // 测试帧保存（预期失败）
    int ret = stream_save_frame(fake_url, test_dir, "jpg", 1, 1000000);
    if (ret == 0) {
        printf("❌ Should have failed with fake URL\n");
        return -1;
    }
    
    printf("✅ Correctly handled invalid RTSP URL for frame saving\n");
    
    // 测试视频保存（预期失败）
    ret = stream_save_video(fake_url, test_dir, "mp4", 10, 1);
    if (ret == 0) {
        printf("❌ Should have failed with fake URL\n");
        return -1;
    }
    
    printf("✅ Correctly handled invalid RTSP URL for video saving\n");
    
    return 0;
}

// 测试参数验证
int test_parameter_validation() {
    printf("\n=== Testing Parameter Validation ===\n");
    
    // 测试 NULL 参数
    StreamConfig config;
    int ret;
    
    ret = stream_parse_config(NULL, &config);
    if (ret == 0) {
        printf("❌ Should reject NULL JSON\n");
        return -1;
    }
    
    ret = stream_parse_config("{}", NULL);
    if (ret == 0) {
        printf("❌ Should reject NULL config pointer\n");
        return -1;
    }
    
    printf("✅ Correctly validates NULL parameters\n");
    
    // 测试 stream_save_frame 参数验证
    ret = stream_save_frame(NULL, "./test", "jpg", 1, 1000);
    if (ret == 0) {
        printf("❌ stream_save_frame should reject NULL input_url\n");
        return -1;
    }
    
    ret = stream_save_frame("test", NULL, "jpg", 1, 1000);
    if (ret == 0) {
        printf("❌ stream_save_frame should reject NULL output_dir\n");
        return -1;
    }
    
    ret = stream_save_frame("test", "./test", NULL, 1, 1000);
    if (ret == 0) {
        printf("❌ stream_save_frame should reject NULL format\n");
        return -1;
    }
    
    printf("✅ stream_save_frame correctly validates parameters\n");
    
    // 测试 stream_save_video 参数验证
    ret = stream_save_video(NULL, "./test", "mp4", 10, 1);
    if (ret == 0) {
        printf("❌ stream_save_video should reject NULL input_url\n");
        return -1;
    }
    
    ret = stream_save_video("test", NULL, "mp4", 10, 1);
    if (ret == 0) {
        printf("❌ stream_save_video should reject NULL output_dir\n");
        return -1;
    }
    
    ret = stream_save_video("test", "./test", NULL, 10, 1);
    if (ret == 0) {
        printf("❌ stream_save_video should reject NULL format\n");
        return -1;
    }
    
    printf("✅ stream_save_video correctly validates parameters\n");
    
    // 测试文件名生成参数验证
    char *filename = stream_gen_filename(NULL, "jpg");
    if (filename != NULL) {
        printf("❌ stream_gen_filename should reject NULL directory\n");
        free(filename);
        return -1;
    }
    
    filename = stream_gen_filename("./test", NULL);
    if (filename != NULL) {
        printf("❌ stream_gen_filename should reject NULL format\n");
        free(filename);
        return -1;
    }
    
    printf("✅ stream_gen_filename correctly validates parameters\n");
    
    return 0;
}

// 演示完整的配置和调用流程
int test_complete_workflow() {
    printf("\n=== Testing Complete Workflow ===\n");
    
    // 1. 解析配置
    const char *config_json = "{"
        "\"format\": \"jpg\","
        "\"outputDir\": \"./workflow_test\","
        "\"frameCount\": 3,"
        "\"frameInterval\": 1000000000,"
        "\"videoNum\": 1"
    "}";
    
    StreamConfig config;
    int ret = stream_parse_config(config_json, &config);
    if (ret != 0) {
        printf("❌ Failed to parse workflow config\n");
        return -1;
    }
    
    printf("✅ Step 1: Config parsed successfully\n");
    
    // 2. 创建输出目录
    if (create_test_dir(config.outputDir) != 0) {
        stream_free_config(&config);
        return -1;
    }
    
    printf("✅ Step 2: Output directory created\n");
    
    // 3. 生成文件名（模拟）
    char *filename = stream_gen_filename(config.outputDir, config.format);
    if (!filename) {
        printf("❌ Failed to generate filename\n");
        stream_free_config(&config);
        return -1;
    }
    
    printf("✅ Step 3: Generated filename: %s\n", filename);
    free(filename);
    
    // 4. 模拟调用流处理函数（实际应用中这里会有真实的 RTSP URL）
    printf("✅ Step 4: Ready for stream processing\n");
    printf("   - Input URL: (would be RTSP stream)\n");
    printf("   - Output Dir: %s\n", config.outputDir);
    printf("   - Format: %s\n", config.format);
    printf("   - Frame Count: %d\n", config.frameCount);
    printf("   - Frame Interval: %d ns\n", config.frameInterval);
    
    // 5. 清理配置
    stream_free_config(&config);
    printf("✅ Step 5: Config cleaned up\n");
    
    return 0;
}

int main() {
    printf("🎬 Stream Module Test Suite\n");
    printf("============================\n");
    
    // 初始化日志（如果需要）
    // log_init();
    
    int total_tests = 0;
    int passed_tests = 0;
    
    // 运行所有测试
    struct {
        const char *name;
        int (*test_func)();
    } tests[] = {
        {"Parameter Validation", test_parameter_validation},
        {"Config Parsing", test_stream_config_parsing},
        {"Filename Generation", test_filename_generation},
        {"Stream Processing Mock", test_stream_processing_mock},
        {"Complete Workflow", test_complete_workflow}
    };
    
    int num_tests = sizeof(tests) / sizeof(tests[0]);
    
    for (int i = 0; i < num_tests; i++) {
        total_tests++;
        printf("\n🧪 Running test: %s\n", tests[i].name);
        
        if (tests[i].test_func() == 0) {
            printf("✅ Test PASSED: %s\n", tests[i].name);
            passed_tests++;
        } else {
            printf("❌ Test FAILED: %s\n", tests[i].name);
        }
    }
    
    // 输出测试结果汇总
    printf("\n" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "=" "\n");
    printf("📊 Test Results Summary\n");
    printf("Total Tests: %d\n", total_tests);
    printf("Passed: %d\n", passed_tests);
    printf("Failed: %d\n", total_tests - passed_tests);
    printf("Success Rate: %.1f%%\n", (float)passed_tests / total_tests * 100);
    
    if (passed_tests == total_tests) {
        printf("🎉 All tests passed!\n");
        return 0;
    } else {
        printf("💥 Some tests failed!\n");
        return 1;
    }
}