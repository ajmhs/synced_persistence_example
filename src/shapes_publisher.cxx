/*
* (c) Copyright, Real-Time Innovations, 2020.  All rights reserved.
* RTI grants Licensee a license to use, modify, compile, and create derivative
* works of the software solely for use with RTI Connext DDS. Licensee may
* redistribute copies of the software provided that all such copies are subject
* to this license. The software is provided "as is", with no warranty of any
* type, including any warranty for fitness for any purpose. RTI is under no
* obligation to maintain or support the software. RTI shall not be liable for
* any incidental or consequential damages arising out of the use or inability
* to use the software.
*/

#include <iostream>
#include <math.h>
#include <dds/pub/ddspub.hpp>
#include <rti/util/util.hpp>      // for sleep()
#include <rti/config/Logger.hpp>  // for logging
// alternatively, to include all the standard APIs:
//  <dds/dds.hpp>
// or to include both the standard APIs and extensions:
//  <rti/rti.hpp>
//
// For more information about the headers and namespaces, see:
//    https://community.rti.com/static/documentation/connext-dds/7.3.0/doc/api/connext_dds/api_cpp2/group__DDSNamespaceModule.html
// For information on how to use extensions, see:
//    https://community.rti.com/static/documentation/connext-dds/7.3.0/doc/api/connext_dds/api_cpp2/group__DDSCpp2Conventions.html

#include "application.hpp"  // for command line parsing and ctrl-c
#include "shapes.hpp"

void run_publisher_application(
    unsigned int domain_id, 
    const std::string& qos_file,
    unsigned int sample_count)
{
    // DDS objects behave like shared pointers or value types
    // (see https://community.rti.com/best-practices/use-modern-c-types-correctly)

    // Read the QoS file provided
    dds::core::QosProvider qos_provider(qos_file);

    // Start communicating in a domain, usually one participant per application
    dds::domain::DomainParticipant participant(
            domain_id,
            qos_provider.participant_qos());

    // Create a Topic with a name and a datatype
    dds::topic::Topic< ::ShapeTypeExtended> topic(participant, "Square");

    // Create a Publisher
    dds::pub::Publisher publisher(participant);

    // Create a DataWriter with the defined QoS
    dds::pub::DataWriter< ::ShapeTypeExtended> writer(publisher, topic, qos_provider.datawriter_qos());

    const int left = 15, top = 15, right = 248, bottom = 278; // limits
    const int shape_size = 30;
    int x = (left + shape_size), y = (bottom - top) / 2; // starting point
    int dx = 1, dy = 1;
    
    int travel_distance = 0;
    float speed_multiplier = 1.0;

    ::ShapeTypeExtended sample;
    sample.color("RED");
    sample.shapesize(shape_size);
    sample.fillKind(ShapeFillKind::SOLID_FILL);
    sample.angle(0);

    // Main loop, write data
    for (unsigned int samples_written = 0;
    !application::shutdown_requested && samples_written < sample_count;
    samples_written++) {
        
        sample.x(x);
        sample.y(y);
                
        std::cout << sample << std::endl;
        writer.write(sample);

        x += dx;
        y += dy;

        // Check for collisions with the bounding box
        if (x >= right - shape_size || x <= left) {
            dx = -dx; // Reverse direction
            speed_multiplier = 2.5;
            travel_distance = 0;
        }

        if (y >= bottom - shape_size || y <= top) {
            dy = -dy;
            speed_multiplier = 2.5;
            travel_distance = 0;
        }

        // Gradually reduce speed_multiplier after an object and a halfs length
        if (speed_multiplier > 1.0) {
            if (++travel_distance > (shape_size * 1.5)) {
                speed_multiplier -= 0.02;
                if (speed_multiplier < 1.0)
                    speed_multiplier = 1.0; 
            }
        }
        
        // You wait, time passes...
        rti::util::sleep(dds::core::Duration::from_millisecs(500 / speed_multiplier));
    }
}

int main(int argc, char *argv[])
{

    using namespace application;

    // Parse arguments and handle control-C
    auto arguments = parse_arguments(argc, argv);
    if (arguments.parse_result == ParseReturn::exit) {
        return EXIT_SUCCESS;
    } else if (arguments.parse_result == ParseReturn::failure) {
        return EXIT_FAILURE;
    }
    setup_signal_handlers();

    // Sets Connext verbosity to help debugging
    rti::config::Logger::instance().verbosity(arguments.verbosity);

    try {
        run_publisher_application(arguments.domain_id, arguments.qos_file, arguments.sample_count);
    } catch (const std::exception& ex) {
        // This will catch DDS exceptions
        std::cerr << "Exception in run_publisher_application(): " << ex.what()
        << std::endl;
        return EXIT_FAILURE;
    }

    // Releases the memory used by the participant factory.  Optional at
    // application exit
    dds::domain::DomainParticipant::finalize_participant_factory();

    return EXIT_SUCCESS;
}
