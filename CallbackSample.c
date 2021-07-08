/******************************************************************************\
* Copyright (C) 2012-2017 Ultraleap Ltd. All rights reserved.                  *
* Ultraleap proprietary and confidential. Not for distribution.                *
* Use subject to the terms of the Leap Motion SDK Agreement available at       *
* https://developer.leapmotion.com/sdk_agreement, or another agreement         *
* between Ultraleap and you, your company or other organization.               *
\******************************************************************************/

#undef __cplusplus

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include "LeapC.h"
#include "ExampleConnection.h"

static LEAP_CONNECTION* connectionHandle;

bool recording = false;
bool policy_wait;
bool write_lock; 

FILE* fp;

void writeLeapVector(LEAP_VECTOR vector) {
    fprintf(fp, ",%f,%f,%f", vector.x, vector.y, vector.z);
}

void writeLeapQuarternion(LEAP_QUATERNION quarternion) {
    fprintf(fp, ",%f,%f,%f,%f", quarternion.w, quarternion.x, quarternion.y, quarternion.z);
}

void writeLeapBone(LEAP_BONE bone) {

    writeLeapVector(bone.prev_joint);
    writeLeapVector(bone.next_joint);
    fprintf(fp, ",%f", bone.width);
    writeLeapQuarternion(bone.rotation);
}

void writeLeapDigit(LEAP_DIGIT digit) {
    fprintf(fp, ",%d", digit.finger_id);
    fprintf(fp,",%u", digit.is_extended);
    writeLeapBone(digit.metacarpal);
    writeLeapBone(digit.proximal);
    writeLeapBone(digit.intermediate);
    writeLeapBone(digit.distal);
}

void writeHeaderLeapVector(const char* name) {
    fprintf(fp,"%s_x", name);
    fprintf(fp,"%s_y", name);
    fprintf(fp,"%s_z", name);
}

void writeHeaderLeapQuarternion(char* name) {
    fprintf(fp,"%s_w", name);
    fprintf(fp,"%s_x", name);
    fprintf(fp,"%s_y", name);
    fprintf(fp,"%s_z", name);
}

void writeHeaderLeapBone(char* name) {

    char temp[100];
    strcpy(temp, name);

    char prev_joint[] = "_prev_joint";
    strcat(temp, prev_joint);
    writeHeaderLeapVector(temp);

    strcpy(temp, name);
    char next_joint[] = "_next_joint";
    strcat(temp, next_joint);
    writeHeaderLeapVector(temp);

    fprintf(fp, "%s_width", name);

    strcpy(temp, name);
    char rotation[] = "_roatation";
    strcat(temp, rotation);
    writeHeaderLeapQuarternion(temp);
}

void writeHeaderLeapDigit(char *name){
    fprintf(fp, "%s_finger_id", name);
    fprintf(fp, "%s_is_extended", name);

    char temp[100];
    strcpy(temp, name);

    char metacarpal[] = "_metacarpal";
    strcat(temp, metacarpal);
    writeHeaderLeapBone(temp);

    strcpy(temp, name);

    char proximal[] = "_proximal";
    strcat(temp, proximal);
    writeHeaderLeapBone(temp);

    strcpy(temp, name);

    char intermediate[] = "_intermediate";
    strcat(temp, intermediate);
    writeHeaderLeapBone(temp);

    strcpy(temp, name);

    char distal [] = "_distal";
    strcat(temp, distal);
    writeHeaderLeapBone(temp);

}

void writeHeader() {
    fprintf(fp, "frame_id,timestamp,tracking_frame_id,nHands");
    
    fprintf(fp, ",hand_id,hand_type,visible_time,pinch_distance,grab_angle,pinch_strength,grab_strength");

    writeHeaderLeapVector(",palm_position");
    writeHeaderLeapVector(",palm_stabilized_position");
    writeHeaderLeapVector(",palm_velocity");
    writeHeaderLeapVector(",palm_normal");
    fprintf(fp,",palm_width");
    writeHeaderLeapVector(",palm_direction");
    writeHeaderLeapQuarternion(",palm_orientation");
 
    
    writeHeaderLeapBone(",arm");
    
    writeHeaderLeapDigit(",thumb");
    writeHeaderLeapDigit(",index");
    writeHeaderLeapDigit(",middle");
    writeHeaderLeapDigit(",ring");
    writeHeaderLeapDigit(",pinky");
    
    fprintf(fp, ",framerate\n");
}

/** Callback for when the connection opens. */
static void OnConnect(){
  //printf("Connected.\n");
}

/** Callback for when a device is found. */
static void OnDevice(const LEAP_DEVICE_INFO *props){
  //printf("Found device %s.\n", props->serial);
}

/** Callback for when a frame of tracking data is available. */
static void OnFrame(const LEAP_TRACKING_EVENT *frame){


    if (recording) {

        // Each hand will have its own frame

        write_lock = true;

        // Data for the frame will be written as follows 
        // frame_id, tracking_frame_id, timestamp, framerate, number of hands

        for (uint32_t h = 0; h < frame->nHands; h++) {

            fprintf(fp,"%lli,%lli,%lli,%i", (long long int)frame->info.frame_id,
                (long long int)frame->info.timestamp,
                (long long int)frame->tracking_frame_id,
                frame->nHands);

            

            LEAP_HAND* hand = &frame->pHands[h];

            // id, type, visible_time, pinch_distance, grab_angle, pinch_strength, grab_strength

            fprintf(fp, ",%i,%s,%lli,%f,%f,%f,%f", hand->id,
                (hand->type == eLeapHandType_Left ? "left" : "right"),
                (long long int)hand->visible_time,
                hand->pinch_distance,
                hand->grab_angle,
                hand->pinch_strength,
                hand->grab_strength);

            // Palm 

            writeLeapVector(hand->palm.position);
            writeLeapVector(hand->palm.stabilized_position);
            writeLeapVector(hand->palm.velocity);
            writeLeapVector(hand->palm.normal);
            fprintf(fp, ",%f", hand->palm.width);
            writeLeapVector(hand->palm.direction);
            writeLeapQuarternion(hand->palm.orientation);
             

            // Arm 

            writeLeapBone(hand->arm);

            // Fingers
            writeLeapDigit(hand->thumb);
            writeLeapDigit(hand->index);
            writeLeapDigit(hand->middle);
            writeLeapDigit(hand->ring);
            writeLeapDigit(hand->pinky);
            
            fprintf(fp, ",%f\n", frame->framerate);

        }

    }

    write_lock = false;
 
}

static void OnLogMessage(const eLeapLogSeverity severity, const int64_t timestamp,
                         const char* message) {
  const char* severity_str;
  switch(severity) {
    case eLeapLogSeverity_Critical:
      severity_str = "Critical";
      break;
    case eLeapLogSeverity_Warning:
      severity_str = "Warning";
      break;
    case eLeapLogSeverity_Information:
      severity_str = "Info";
      break;
    default:
      severity_str = "";
      break;
  }
  //printf("[%s][%lli] %s\n", severity_str, (long long int)timestamp, message);
}

static void* allocate(uint32_t size, eLeapAllocatorType typeHint, void* state) {
  void* ptr = malloc(size);
  return ptr;
}

static void deallocate(void* ptr, void* state) {
  if (!ptr)
    return;
  free(ptr);
}

/*
static void onPolicy(const uint32_t current_policies) {
    if ((current_policies | eLeapPolicyFlag_OptimizeHMD) == current_policies) {
        printf("\nTracking optimized for head - mounted device, True\n");
    }
    else {
        printf("\nTracking optimized for head - mounted device, False\n");
    }

    policy_wait = false;
     
    return;
}
*/

void get_lms_status(LEAP_CONNECTION* connectionHandle);
void get_num_devices(LEAP_CONNECTION* connectionHandle);
void get_timestamp(void);
void set_filename(void);
void start_recording(void);
void stop_recording(void);


int main(int argc, char** argv) {

  //Set callback function pointers
  ConnectionCallbacks.on_connection          = &OnConnect;
  ConnectionCallbacks.on_device_found        = &OnDevice;
  ConnectionCallbacks.on_frame               = &OnFrame;
  ConnectionCallbacks.on_log_message         = &OnLogMessage;
  //ConnectionCallbacks.on_policy              = &onPolicy;

  connectionHandle = OpenConnection();
  {
    LEAP_ALLOCATOR allocator = { allocate, deallocate, NULL };
    LeapSetAllocator(*connectionHandle, &allocator);
  }

  LeapSetPolicyFlags(*connectionHandle, eLeapPolicyFlag_Images | eLeapPolicyFlag_MapPoints | eLeapPolicyFlag_OptimizeHMD, 0);
  
  bool running = true;
  
  int choice;

  printf("Welcome to the Leap Motion Recorder Software, please pick a choice\n\n");
  printf("1. Get Leap motion service status\n");
  printf("2. Get Number of devices\n");
  printf("3. Get Timestamp\n");
  printf("4. Set Filename\n");
  printf("5. Start Recording\n");
  printf("6. Stop Recording\n");
  printf("7. Exit\n");

  do {
    printf(">");
     
    scanf("%d", &choice);

    switch (choice) {
        case 1: 
            get_lms_status(connectionHandle);
            break;
        case 2: 
            get_num_devices(connectionHandle);
            break;
        case 3:
            get_timestamp();
            break;
        case 4:
            set_filename();
            break;
        case 5:
            start_recording();
            break;
        case 6:
            stop_recording();
            break;
        case 7: printf("Program exiting\n");
            break;
        default: printf("Wrong Choice. Enter again\n");
            break;
    }

  } while (choice != 7);

  //DestroyConnection();

  return 0;
}


void get_lms_status(LEAP_CONNECTION* connectionHandle) {
    /*
    eLeapConnectionStatus 
     0 = Not Connected
     1 = Connected
     2 = HandshakeIncomplete
     0xE7030004 = Not Running 
    */
    
    LEAP_CONNECTION_INFO connectionInfo;
    eLeapRS return_connection;
    return_connection = LeapGetConnectionInfo(*connectionHandle, &connectionInfo);

    eLeapConnectionStatus status;
    status = connectionInfo.status;
    
    printf("Staus of leap service is, %d\n", status);
}

void get_num_devices(LEAP_CONNECTION* connectionHandle) {
    LEAP_DEVICE_REF deviceArray;
    uint32_t nDevices;
    eLeapRS return_result;

    return_result = LeapGetDeviceList(*connectionHandle, &deviceArray, &nDevices);
    printf("Number of devices is, %d\n", nDevices);

}

void get_timestamp(void) {

    /// <summary>
    /// Prints the system timestamp in UTC, and the Leap Timestamp
    /// </summary>
   
    struct timespec ts;
    int64_t leap_time;

    timespec_get(&ts, TIME_UTC);
    leap_time = LeapGetNow();

    printf("Leap timestamp, ");
    printf("%" PRId64 "\n", leap_time);

    // Time format 
    // YYYY-MM-DD HH:MM:SS
    // Seconds to 6 dp (microseconds)
    long msec = ts.tv_nsec / 1000;

    char buff[100];
    strftime(buff, sizeof buff, "%F %T", gmtime(&ts.tv_sec));
    printf("Current time, %s.%06ld\n", buff, msec);
}

void set_filename(void) {
    char filename[100];

    printf(">");
    scanf("%s", filename);


    fp = fopen(filename, "w");
    writeHeader();
}

void start_recording(void) {
    recording = true;
}

void stop_recording(void) {
    recording = false;

    while (write_lock) {
        // Wait until the file can be closed
    }

    fclose(fp);
    fp = NULL;
}