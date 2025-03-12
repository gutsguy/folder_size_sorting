#include <iostream>
#include <filesystem>
#include <vector>
#include <algorithm>
#include <thread>
#include <atomic>
#include <windows.h>

namespace fs = std::filesystem;
using namespace std::chrono_literals;

// 파일 및 폴더 정보를 저장하는 구조체
struct FileInfo {
    std::string name;
    uintmax_t size;

    // 정렬을 위한 비교 연산자 (내림차순 정렬)
    bool operator<(const FileInfo& other) const {
        return size > other.size;
    }
};

// 폴더 내 모든 파일 및 폴더 크기를 계산하는 함수
std::vector<FileInfo> get_directory_sizes(const std::string& path) {
    std::vector<FileInfo> files;
    
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            uintmax_t size = 0;

            if (fs::is_regular_file(entry)) {
                size = fs::file_size(entry);
            } else if (fs::is_directory(entry)) {
                // 폴더 크기 계산
                for (const auto& sub_entry : fs::recursive_directory_iterator(entry, fs::directory_options::skip_permission_denied)) {
                    if (fs::is_regular_file(sub_entry)) {
                        size += fs::file_size(sub_entry);
                    }
                }
            }

            files.push_back({entry.path().filename().string(), size});
        }
    } catch (const std::exception& e) {
        std::cerr << "오류 발생: " << e.what() << std::endl;
    }

    return files;
}

// Windows 경로를 WSL 경로로 변환하는 함수
std::string convert_windows_to_wsl(const std::string &win_path)
{
    if (win_path.length() > 2 && win_path[1] == ':')
    {
        std::string wsl_path = "/mnt/" + std::string(1, std::tolower(win_path[0])) + win_path.substr(2);
        std::replace(wsl_path.begin(), wsl_path.end(), '\\', '/');
        return wsl_path;
    }
    return win_path;
}

// 로딩 애니메이션 (별도 스레드)
std::atomic<bool> loading_done(false); // 로딩 종료 여부
void loading_animation()
{
    const char spinner[] = {'|', '/', '-', '\\'};
    int index = 0;
    while (!loading_done)
    {
        std::cout << "\r[계산 중... " << spinner[index] << "]" << std::flush;
        index = (index + 1) % 4;
        std::this_thread::sleep_for(100ms);
    }
    std::cout << "\r[계산 완료!]   \n";
}

int main()
{
    // Windows 콘솔에서 UTF-8 설정
    SetConsoleOutputCP(CP_UTF8);

    std::string path;
    std::cout << "경로를 입력하세요: ";
    std::getline(std::cin, path);

    // Windows 경로라면 WSL 경로로 변환
    //path = convert_windows_to_wsl(path);

    if (!fs::exists(path) || !fs::is_directory(path))
    {
        std::cerr << "잘못된 경로입니다: " << path << std::endl;
        return 1;
    }

    // 로딩 애니메이션 시작 (별도 스레드)
    std::thread loading_thread(loading_animation);

    // 디렉토리 파일 크기 가져오기
    std::vector<FileInfo> files = get_directory_sizes(path);

    // 로딩 애니메이션 종료
    loading_done = true;
    loading_thread.join();

    // 크기 순으로 정렬
    std::sort(files.begin(), files.end());

    // 출력
    std::cout << "\n=== 파일 및 폴더 크기 ===\n";
    for (const auto &file : files)
    {
        std::cout << file.name << " <<< " << file.size / (1024 * 1024) << " MB" << std::endl;
    }

    std::cin.get();
    return 0;
}
