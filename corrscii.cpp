#include <iostream>

#include <string_view>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <random>
#include <ctime>

#include <opencv2/core/utility.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core.hpp>

class CorrSCII {
    public:
        CorrSCII() {
            loadSpectrumFile();
            srand(time(NULL));
        };

        int toCorrscii (std::string target_path, std::string save_path, int char_width, int char_height) {
            std::cout << "Setting up variables. . ." << std::endl;

            working_video.open(target_path);
            if (!working_video.isOpened()) { std::cout << "Error: cannot open targetted video" << std::endl; return -1; };

            if (char_height == 0) {char_height = working_video.get(cv::CAP_PROP_FRAME_HEIGHT)/5; };
            if (char_width == 0) { char_width = working_video.get(cv::CAP_PROP_FRAME_WIDTH)/5; };
            if (save_path == "") { save_path = working_directory; };

            startup(char_width, char_height);

            cv::VideoWriter result_video(
                cv::String(save_path+"\\CorrSCII_Result.mp4"),
                working_video.get(cv::CAP_PROP_FOURCC),
                working_video.get(cv::CAP_PROP_FPS),
                cv::Size(video_width, video_height),
                false
            );
            if (!result_video.isOpened()) { std::cout << "Error: cannot initialize VideoWriter" << std::endl; return -1;};

            int frame_count = 0;

            std::cout << "Variables successfully decalred." << std::endl;
            while (working_video.read(working_image)) {
                if (working_image.empty()) { break; };

                std::cout << "Frames [" << frame_count << "/" << video_frames << "]\r";

                std::string corrscii_equivelant;
                for (int y = 0; y < working_image.rows; y += y_iteration) {
                    for (int x = 0; x < working_image.cols; x += x_iteration) {
                        corrscii_equivelant += findCorrsciiEquivelant(getAverageColor(x, y));
                    };
                    corrscii_equivelant += "\n";
                };

                cv::Mat converted(working_image.rows, working_image.cols, CV_8UC1, cv::Scalar(0, 0, 0));
                
                int y_pos = 0;
                for (std::string line : splitString(corrscii_equivelant, "\n")) {
                    double dilute = diluteFont(line);
                    cv::putText(converted, line, cv::Point(0, y_pos), working_font, dilute, cv::Scalar(255, 255, 255));

                    y_pos += getFontSize(line, dilute).height;
                };

                result_video.write(converted);
                frame_count++;
            };

            working_video.release();
            result_video.release();

            std::cout << "Frames [" << video_frames << "/" << video_frames << "] FINISHED\nVideo saved to " << save_path << std::endl;
            return 0;
        };

    private:
        cv::Size getFontSize(const std::string content, const double dilution = 1) {
            return cv::getTextSize(content, working_font, dilution, 1, NULL);
        };

        double diluteFont(const std::string content, int font_width = 0) {
            if (font_width == 0) { font_width = getFontSize(content, 1).width; };
            return double(video_width)/font_width;
        };

        int getRand(const int min, const int max) {
            if (min > max) { throw std::invalid_argument(" arguement 'min' cannot be greater than arguement 'max'\nmin: "+std::to_string(min)+"\nmax: "+std::to_string(max)); };
            if (min == max) { return min; };

            return min + rand() % (max - min);
        };

        int getAverageColor(const int x_starting, const int y_starting) {
            int x_end = (x_starting + x_iteration <= working_image.cols) ? x_starting + x_iteration : working_image.cols;
            int y_end = (y_starting + y_iteration <= working_image.cols) ? y_starting + y_iteration : working_image.rows;

            int average = 0;
            for (int y = y_starting; y < y_end; y++) {
                for (int x = x_starting; x < x_end; x++) {
                    average += (int)(working_image.at<cv::Vec3b>(y, x).val[0]);
                };
            };

            return average/((x_end-x_starting)*(y_end-y_starting));
        };

        std::string findCorrsciiEquivelant(const int color){
            std::string similar_characters = "";

            for (int i = 0; i < spectrum_chars.size(); i++) {
                std::vector<int> range = spectrum_vals[i];
                
                if ((unsigned)(color-range[0]) <= (range[1]-range[0])) {
                    similar_characters += spectrum_chars[i];
                };
            };

            return (similar_characters.empty()) ? "\u2588" : std::string(1, similar_characters[getRand(0, similar_characters.size()-1)]);
        }

        std::vector<std::string> splitString(std::string input_string, const std::string split_at_seq) {
            std::vector<std::string> __return;
            
            while (std::size_t __foundat = input_string.find(split_at_seq)) {
                if (__foundat == std::string::npos) {
                    __return.push_back(input_string);
                    break;
                };

                __return.push_back(std::string(input_string.begin(), input_string.begin() + __foundat));
                input_string = input_string.substr(__foundat + split_at_seq.size());
            };

            return __return;
        };

        void loadSpectrumFile() {
            std::ifstream spectrum_file(working_directory+"\\spectrum.txt");
            if (!spectrum_file.is_open()) { spectrum_file.close(); throw std::invalid_argument(" can't open spectrum file"); };

            std::string line;
            while (std::getline(spectrum_file, line)) {
                std::vector<std::string> x = splitString(line, " : ");

                spectrum_chars += x[1];

                std::vector<std::string> range = splitString(x[0], ", ");
                spectrum_vals.push_back({std::stoi(range[0]), std::stoi(range[1])});
            };

            spectrum_file.close();
            if (spectrum_chars.empty() || spectrum_vals.empty()) { throw std::invalid_argument(" spectrum data could not written to variable(s)"); };
        };

        void startup(const int ch_w, const int ch_h, const int font = cv::FONT_HERSHEY_DUPLEX) {
            working_font = font;
            char_height = ch_h;
            char_width = ch_w;

            video_frames = working_video.get(cv::CAP_PROP_FRAME_COUNT);
            video_height = working_video.get(cv::CAP_PROP_FRAME_HEIGHT);
            video_width = working_video.get(cv::CAP_PROP_FRAME_WIDTH);

            x_iteration = double(video_height)/char_height;
            y_iteration = double(video_width)/char_width;
        };

        cv::VideoCapture working_video;
        cv::Mat working_image;

        int working_font;
        int video_frames;
        int video_height;
        int video_width;
        int x_iteration;
        int y_iteration;
        int char_height;
        int char_width;

        std::string working_directory = std::filesystem::current_path().string();
        std::vector<std::vector<int>> spectrum_vals;
        std::string spectrum_chars;
};

int main(int argc, char* argv[]) {
    std::cout << "Pass '--help' to view the help menu" << std::endl;
    if (argc == 1) { return 0; };

    using namespace std::literals;

    if (argv[1] == "--help"sv) {
        std::cout
        << "CorrSCII Help Menu\n" << std::endl
        << "About: The CorrSCII algorithm takes in a path to a video, a save path for the result, and dimenions. About CorrSCII's use, please see a list of arguements below:\n" << std::endl
        << "Format: [arguement] [parameters]\n" << std::endl
        << "Arguements:" << std::endl
        << "\t[-targ] : Path to the target video" << std::endl
        << "\t[-save] : Path to save the resulting video. Defaults to the executable's path" << std::endl
        << "\t[-resx] : Sets how many characters the resulting video's width should be--defaults to [video width]/5" << std::endl
        << "\t[-resy] : Sets how many characters the resulting video's height should be--defaults to [video height]/5" << std::endl
        << "\t[--res] : Returns resolution of target video. Does not initiate conversion" << std::endl
        << "\t[--help]: Displays help menu\n" << std::endl;
        return 0;
    };

    std::string target_path = "";
    std::string save_path = "";
    int char_width = 0;
    int char_height = 0;

    bool return_resolution = false;
    bool skip = false;

    for (int i = 1; i < argc-1; i++) {
        if (skip) { skip = !skip; continue; };

        std::string arg = argv[i];
        std::string param = (i+1 <= argc-1) ? argv[i+1] : "";

        if (arg == "-targ"sv) {
            target_path = param; skip = !skip;
        } else if (arg == "-save"sv) {
            save_path = param; skip = !skip;
        } else if (arg == "-resx"sv) {
            char_width = std::stoi(param); skip = !skip;
        } else if (arg == "-resy"sv) {
            char_height = std::stoi(param); skip = !skip;
        } else if (arg == "--res"sv) {
            return_resolution = !return_resolution;
        } else {
            std::cout << "Error: command '" << arg << "' not recognized" << std::endl; return -1;
        };
    };

    if (return_resolution) {
        if (target_path == "") { std::cout << "Error: cannot get resolution of undefined target path" << std::endl; return -1; };

        cv::VideoCapture vid(target_path);
        std::cout << vid.get(cv::CAP_PROP_FRAME_WIDTH) << "x" << vid.get(cv::CAP_PROP_FRAME_HEIGHT) << std::endl;
        return 0;
    };

    if (target_path == "") { std::cout << "Error: cannot preform conversion with empty target path" << std::endl; return -1; };
    
    CorrSCII corr;
    return corr.toCorrscii(target_path, save_path, char_width, char_height);
};
