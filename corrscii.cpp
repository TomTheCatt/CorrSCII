#include <iostream>

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
            if (save_path == "") { save_path = working_directory; };

            working_video.open(target_path);
            if (!working_video.isOpened()) { std::cout << "Error: cannot open targetted video" << std::endl; return -1; };

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

            std::cout << "Variables successfully decalred. Starting conversion. . ." << std::endl;
            while (working_video.read(working_image)) {
                if (working_image.empty()) { break; };
                cv::cvtColor(working_image, working_image, cv::COLOR_BGR2GRAY);

                //std::cout << "Frames [" << frame_count << "/" << video_frames << "]\r";

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

            if (!similar_characters.empty()) {
                return std::string(1, similar_characters[getRand(0, similar_characters.size()-1)]);
            } else { return "\u2588"; };
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

int main() {
    std::string target_path = std::filesystem::current_path().string()+"/test_target.mp4";
    std::string save_path = "";
    int char_width = 128;
    int char_height = 72;
    
    CorrSCII corr;
    return corr.toCorrscii(target_path, save_path, char_width, char_height);
};