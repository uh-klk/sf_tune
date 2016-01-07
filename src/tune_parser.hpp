//TuneParcer: Reads tune from a XML file, interpretes and returns
// a sequence of tones at a time taking into account of the sequence's
// timing
// Todo: - taking user preferences into account (preference from topic to compile the tune)
//       - better name for TuneTargetState

#ifndef TUNE_PARSER_HPP_INCLUDED
#define TUNE_PARSER_HPP_INCLUDED

#include <iostream>
#include <vector>
#include <fstream>
#include <string.h>
#include <cstring>
#include <unistd.h>

#include <time.h>

#include "rapidxml/rapidxml.hpp"

using namespace rapidxml;
using namespace std;

struct TuneTargetState { //ToneTargetState
    int msec;
    int note;
    int modifierFlag; //to indicate if this tune should be modify based on user's preferences
};

struct TonesSequence { //A sequence of tones
    vector<TuneTargetState> tuneTargetState;
    int numberOfTones; //numberOfTones
};

struct Tune { //XML structure
    int numberOfSequences;
    int msecModifier[3];
    int noteModifier[3];
    vector<TonesSequence> tSeq;
    string id;
    string type;
    int userPreference;
};

class TuneParser {

public:
    TuneParser(string xmlPath);
    ~TuneParser(){}

    void setupTune(Tune *tune , char *xml_filename); //xml in default folder
    void setupTuneWithPath(Tune *tune, char *xml_fullpath_filename); //full path to xml

    void resetTune();
    bool playTune(Tune *tune, char * tonesArray, int *tonesSize);  //read a selected tune
    void playSequenceOfTones(TonesSequence *tSeq, char * tonesArray, int *tonesSize); //read a sequence of tune

    void requestTune(char *xml_filename)
    {
        resetTune(); //reset to play new behaviour
        setupTune(&requestedTune, xml_filename); //initiliased the request behaviour
    }

    bool playRequestedTune(char * tonesArray, int *tonesSize)
    {
        if (playTune(&requestedTune, tonesArray, tonesSize))
        {
            return true;
        }
        return false;
    }

    unsigned long long getTimeMs_ull()
    {
        static struct timeval tp;
        gettimeofday(&tp, NULL);
        unsigned long long ms =
                (unsigned long long) tp.tv_sec*1000 +
                (unsigned long long) tp.tv_usec/1000;

        return ms;
    }

    unsigned int getTimeMs()
    {
        return (unsigned int) (getTimeMs_ull()-objStartMs);
    }


private:

    string default_xml_path_;
    Tune Happy, Excited, Bored, Tired;
    Tune requestedTune;
    unsigned long long objStartMs;
    //unsigned long long myNextSequenceTimer, myInitSeqTimer;
    unsigned int myNextSequenceTimer, myInitSeqTimer;
    int myTonesSequencesCounter;
    char tonesArray_[40];
    int tonesSize_;

};

TuneParser::TuneParser(string default_xml_path) :  default_xml_path_(default_xml_path)
{
  myNextSequenceTimer = 0;
  myInitSeqTimer = 0;
  myTonesSequencesCounter = 999;
    
  int tuneArraySize = 0;

  objStartMs = getTimeMs_ull(); //to use current time instead of time since 1970

}

/*
 * Open a tune xml file to read the its contents and set up the tune
 */
void TuneParser::setupTune(Tune *tune , char *xml_filename)
{
  char xml_fullpath_filename[120];
  strcpy (xml_fullpath_filename, default_xml_path_.c_str());   //path to the tune folders
  strcat (xml_fullpath_filename,xml_filename);          //tune filename in XML format
  setupTuneWithPath(tune, xml_fullpath_filename);     //import tune from the XML file
}


void TuneParser::setupTuneWithPath(Tune *tune, char *xml_fullpath_filename)
{
  ifstream *ifs;
  string line;
  string buf;
  char *doc;

  int i ,j;

  string id, type, userPreference, m_msec1, m_msec2, m_msec3, m_note1, m_note2, m_note3;

  id = "id"; type = "type"; userPreference = "userPreference";
  m_msec1 = "m_msec1";  m_msec2 = "m_msec2";  m_msec3 = "m_msec3"; 
  m_note1 = "m_note1";  m_note2 = "m_note2";  m_note3 = "m_note3";
  
  string msec, note, modifierFlag;
  msec = "msec";  note = "note";  modifierFlag = "md";

  TonesSequence *tSeq;
  TuneTargetState *tuneTargetState;

  ifs = new ifstream(xml_fullpath_filename);
 
  if( !ifs->good() )
  {
    printf("Invalid filename: %s\n", xml_fullpath_filename);
    exit(1);
  }
  
  while( ifs->good() )
  { 
    while(getline(*ifs, line))  //copy xml contents into buf
      buf += line;
  
    doc = new char[buf.length()+1];
    strcpy(doc, buf.c_str());

    xml_document<> behaviour_xml;
    behaviour_xml.parse<0>(doc);
    xml_node<> *behNode = behaviour_xml.first_node(); //Behaviour
    
    id = behNode->first_attribute()->value();
    type = behNode->last_attribute()->value();

    for( xml_attribute<> *attribute = behNode->first_attribute(); attribute; attribute = attribute->next_attribute())
    {
          if( !id.compare(attribute->name()) )
            tune->id = attribute->value();
          else if( !type.compare(attribute->name()) )
            tune->type = attribute->value();
          else if( !userPreference.compare(attribute->name()) )
            tune->userPreference = atoi(attribute->value());
    }

    for( xml_node<> *channel = behNode->first_node(); channel; channel = channel->next_sibling()) //each expressive channel
    {
        for( xml_attribute<> *attribute = channel->first_attribute(); attribute; attribute = attribute->next_attribute())
        {
            if( !m_msec1.compare(attribute->name()) )
                tune->msecModifier[0] = atoi(attribute->value());
            else if( !m_msec2.compare(attribute->name()) )
                tune->msecModifier[1] = atoi(attribute->value());
            else if( !m_msec3.compare(attribute->name()) )
                tune->msecModifier[2] = atoi(attribute->value());
            else if( !m_note1.compare(attribute->name()) )
                tune->noteModifier[0] = atoi(attribute->value());
            else if( !m_note2.compare(attribute->name()) )
                tune->noteModifier[1] = atoi(attribute->value());
            else if( !m_note3.compare(attribute->name()) )
                tune->noteModifier[2] = atoi(attribute->value());
        }

       // printf("\nsequence size %d\n",(int) tune->tSeq.size());
        if (tune->tSeq.size()!=0) //if the vector container is not empty
            tune->tSeq.erase(tune->tSeq.begin(),tune->tSeq.end()); //clear its contents

        i = 0;  //reset the sequence counter before begin
        for( xml_node<> *sequence = channel->first_node(); sequence; sequence = sequence->next_sibling())  //each sequence
        { //read each sequence one by one
          tSeq = new TonesSequence; //create a new sequence container
          tune->tSeq.push_back(*tSeq);   //add the container as a new sequence
          j = 0;  //reset the joint to zero after we have collected all 4 joints of a sequence 
          for(xml_node<> *joint = sequence->first_node(); joint; joint = joint->next_sibling())  //each joint
          { //read each joint one by one            
            tuneTargetState = new TuneTargetState; //create a new joint container
            tune->tSeq[i].tuneTargetState.push_back(*tuneTargetState); //add the joint container to servoTargetState
            
            for(xml_attribute<> *attribute = joint->first_attribute(); attribute; attribute = attribute->next_attribute()) 
            { //read each attribute of a joint one by one
              if( !msec.compare(attribute->name()) )
                tune->tSeq[i].tuneTargetState[j].msec = atoi(attribute->value());
              else if( !note.compare(attribute->name()) )
                tune->tSeq[i].tuneTargetState[j].note = atoi(attribute->value());
              else if( !modifierFlag.compare(attribute->name()) )
                tune->tSeq[i].tuneTargetState[j].modifierFlag = atoi(attribute->value());
            }
            //printf("\n i = %d", i); printf("\n j = %d", j);
            //printf("\n %d, %d, %d", tune->tSeq[i].servoTargetState[j].goal, tune->tSeq[i].servoTargetState[j].speed, tune->tSeq[i].servoTargetState[j].modifierFlag);
            j++;  //proceed to next joint/action 
          }
          tune->tSeq[i].numberOfTones = j;
          i++; //proceed to next sequence
        }
        tune->numberOfSequences = i;
      }

    ifs->close();
  }
}

/******************************************************************************
*   The method is call to reset/reinitialise the tune behaviour so that a new
*   tune can be executed.
*   @author Kheng Lee Koay
*   @param
*   @date 22/12/2011
******************************************************************************/
void TuneParser::resetTune()
{
  myNextSequenceTimer = 0;
  myTonesSequencesCounter = 0;
}
/******************************************************************************
*   The method is use to execute the midi tune behaviour. It can handle a
*   sequence or multiple sequences of midi tune. It will complete a sequence
*   before proceeding to the next sequence, unless a resetBehaviour() is call
*   prior to executing a new tune behaviour. This function should be called from
*   within a loop or in a thread where it get call all the time so that it have
*   the opportunity to execute all the sequences of a tune behaviour.
*
*
*   @author Kheng Lee Koay
*   @param 
*   @date 22/12/2011
******************************************************************************/
bool TuneParser::playTune(Tune *tune, char * tonesArray, int *tonesSize)
{

  if( myTonesSequencesCounter < tune->tSeq.size() ) //play a sequence of tones at a time
    if( myNextSequenceTimer < getTimeMs()-myInitSeqTimer)
    {//wait till previous sequence completed before playing next sequence
      playSequenceOfTones(&tune->tSeq[myTonesSequencesCounter], tonesArray, tonesSize);
      //recerve a reference of the tune  
      myInitSeqTimer = getTimeMs();
      myTonesSequencesCounter++; //is reset by calling initBehaviour

      return true;
    }

  return false;
}

/******************************************************************************
*   The method compose the sequence of tune and send it to the driver.
*   @author Kheng Lee Koay
*   @param 
*   @param requestCheck 
*   @date 22/12/2011

******************************************************************************/
void TuneParser::playSequenceOfTones(TonesSequence *tSeq, char * tonesArray, int *tonesSize)
{
  char tArray[40];
  int tSize;

  myNextSequenceTimer = 0; //this is to reset for 2nd sequence onwards
  for(int i=0; i< tSeq->tuneTargetState.size(); i++)  {
    tArray[i*2] = (char) tSeq->tuneTargetState[i].msec;
    tArray[i*2+1] = (char) tSeq->tuneTargetState[i].note;
    myNextSequenceTimer+= tArray[i*2]; //stored the total time required to finish this tune
  }	     
 
  myNextSequenceTimer = myNextSequenceTimer*30;  //total duration to wait for a sequence to finish
  tSize = tSeq->tuneTargetState.size()*2;

  *tonesSize = tSize; //passing back the size

  for(int i=0; i<tSize; i++)
    tonesArray[i]=tArray[i]; //and the tone

}

#endif
