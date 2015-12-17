// - xml folder should be in the same directory as the generated exefile
//rostopic pub /sf_tune/sf_tune_filename std_msgs/String Happy.xml

#include "ros/ros.h"
#include "tune_parser.hpp"
#include "std_msgs/String.h"  //for receiving filename 
#include "std_msgs/ByteMultiArray.h" //for sending and receiving 40 chars of midi

class SFTune
{
public:

    SFTune(string xml_path, ros::NodeHandle nh) : xml_path_(xml_path), node_(nh)
    {
        //TODO: need create a param for changing the path

        tune_parser_ = new TuneParser(xml_path_ );
    }
    ~SFTune() {}

    void init();

    void receiveCB(const std_msgs::String::ConstPtr& msg);

    void requestTune(char *xml_filename)
    {
         tune_parser_->requestTune(xml_filename);
    }

    void playRequestedTune();

protected:
    ros::NodeHandle node_;
    ros::Subscriber subscriber_;
    ros::Publisher publisher_;
    string xml_path_;
    TuneParser *tune_parser_;
};

void SFTune::init()
{
    subscriber_ = node_.subscribe("sf_tune_filename", 10, &SFTune::receiveCB, this);
    publisher_ = node_.advertise<std_msgs::ByteMultiArray>("/RosAria/aria_midi", 1000);
}

void SFTune::receiveCB(const std_msgs::String::ConstPtr& msg)
{
    char *xml_filename;
    ROS_INFO("I heard mode: [%s]", msg->data.c_str()); //filename
    //setup the tune to be played
    xml_filename = (char *) msg->data.c_str();
    requestTune(xml_filename);
}

void SFTune::playRequestedTune()
{
    char tonesArray[40];
    int tonesSize;

    std_msgs::ByteMultiArray tuneArrayMsg;
    tuneArrayMsg.data.clear();

    if (tune_parser_->playRequestedTune(tonesArray, &tonesSize))
    {
        printf("\nReceived Tones: size is = %d, tones are:\n", tonesSize);

        for (int i=0; i<tonesSize; i++)
        {
            printf("%d, ", tonesArray[i]);
            tuneArrayMsg.data.push_back(tonesArray[i]);
        }

        publisher_.publish(tuneArrayMsg);
    }
}

int main(int argc, char** argv)
{
    ros::init(argc, argv, "sf_tune");
    ros::NodeHandle n(std::string("~"));

    SFTune *sf_tune = new SFTune("behaviourXML/tune/", n);

    sf_tune->init(); //init subscriber and publisher

    ros::Rate loop_rate(100);

    while (ros::ok())
    {
        ros::spinOnce();
        loop_rate.sleep();
        sf_tune->playRequestedTune();
    }

    delete sf_tune;

    return 0;
}
