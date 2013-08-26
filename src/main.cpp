/************************************
Copyright (C) 2013 Kevin Allen

This file is part of kacqtan.

kacqtan is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

kacqtan is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with kacqtan.  If not, see <http://www.gnu.org/licenses/>.

Date: 01.08.2010
*************************************/
#include "main.h" // all functions are declared there
#include "mainWindow.h"
#include "rhd2000evalboard.h"
#include <gtkmm/application.h>
// functions to print information to terminal
void print_options();
void print_version();
void print_help();
void openInterfaceBoard();
int main (int argc, char *argv[])
{
  int non_opt_arguments; // given by user
  int num_arg_needed=0; // required by program
  // to get the options
  int opt; 
  int i;
  char * terminal_configuration_file; // configuration file to run kacqtan in terminal mode
  // flag for each option
  int with_h_opt=0; // help
  int with_v_opt=0; // version
  int with_t_opt=0; // terminal
  // get the options using the gnu getop_long function
  while (1)
    {
      static struct option long_options[] =
	{
	  {"help", no_argument,0,'h'},
	  {"version", no_argument,0,'v'},
	  {"terminal", no_argument,0,'t'},
	  {0, 0, 0, 0}
	};
      int option_index = 0;
      opt = getopt_long (argc, argv, "hvt",
			 long_options, &option_index);
      /* Detect the end of the options. */
      if (opt == -1)
	break;
      
      switch (opt)
	{
	case 0:
	  /* If this option set a flag, do nothing else now. */
	  if (long_options[option_index].flag != 0)
	    break;
	  printf ("option %s", long_options[option_index].name);
	  if (optarg)
	    printf (" with arg %s", optarg);
	  printf ("\n");
	  break;
	case 'h':
	  {
	    with_h_opt=1;
	    break;
	  }
	case 'v':
	  {
	    with_v_opt=1;
	    break;
	  }
	case 't':
	  {
	    with_t_opt=1;
	    break;
	  }
	case '?':
	  /* getopt_long already printed an error message. */
	  //	  break;
	default:
	  return 1;
	}
    }
  

  // check if user gave the right number of arguments
  non_opt_arguments=argc-optind; // number of non-option argument required
  if ((non_opt_arguments)!=num_arg_needed)
    {
      fprintf(stderr,"Usage for %s is \n", PACKAGE_NAME); // PACKAGE_NAME is defined in config.h, set by autotools
      fprintf(stderr,"%s\n",PACKAGE_NAME);
      print_options();
      fprintf(stderr,"You need %d arguments but gave %d arguments: \n",num_arg_needed,non_opt_arguments);
      for (i = 1; i < argc; i++)
	{
	  fprintf(stderr,"%s\n", argv[i]);
	}
      return (1);
    }  

  // if --version or -v given
  if (with_v_opt) // print the version information
    {
      print_version();
      return 0;
    }
  // if --help or -h given
  if (with_h_opt) // print the help information
    {
      print_help();
      return 0;
    }
  if(with_t_opt)
    {
      fprintf(stderr,"%s will run in the terminal\n",PACKAGE_NAME);
      openInterfaceBoard();
    }
  

  // to get a window derived from the builder's window see
  //https://developer.gnome.org/gtkmm-tutorial/3.2/sec-builder-using-derived-widgets.html.en


  Glib::RefPtr<Gtk::Application> app =
    Gtk::Application::create(argc, argv,"org.gtkmm.examples.base");

  //Load the GtkBuilder file and instantiate its widgets:
  Glib::RefPtr<Gtk::Builder> refBuilder = Gtk::Builder::create();
  try
    {
      refBuilder->add_from_file("kacqtan.glade");
    }
  catch(const Glib::FileError& ex)
    {
      std::cerr << "FileError: " << ex.what() << std::endl;
      return 1;
    }
  catch(const Glib::MarkupError& ex)
    {
      std::cerr << "MarkupError: " << ex.what() << std::endl;
      return 1;
    }
  catch(const Gtk::BuilderError& ex)
    {
      std::cerr << "BuilderError: " << ex.what() << std::endl;
      return 1;
    }
  
  mainWindow window;
  return app->run(window);

  //Gtk::Main kit(argc, argv); // should be call in all gtkmm applications

  // we should build our main window here.
  
  // Gtk::Window window;
  //Gtk::Main::run(window);
  return 0;
 }


void print_version()
{
  printf("%s %s\n",PACKAGE_NAME,PACKAGE_VERSION);
  printf("%s\n",PACKAGE_COPYRIGHT);
  printf("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n");
  return;
}
void print_help()
{
  printf("\n");
  printf("%s %s is a program to record physiological data on Gnu/Linux computers and is available at %s\n\n",PACKAGE_NAME,PACKAGE_VERSION,PACKAGE_URL);
  printf("When executed without option or argument, kacqtan starts its graphical user interface. Use the following options to run in the terminal mode.\n\n");
  print_options();
  printf("\n");
  printf("report bugs: %s\n\n",PACKAGE_BUGREPORT);
  printf("more information: %s\n\n",PACKAGE_URL);
  return;
}
void print_options()
{
  printf("possible options:\n");
  printf("--version or -v\t\t: print the program version\n");
  printf("--help or -h\t\t: will print this text\n");
  return;
}



void openInterfaceBoard()
{
  // function called from the gui to set up the board
  // so that it is ready to record.
  Rhd2000EvalBoard *evalBoard;
  int errorCode=0;
  evalBoard = new Rhd2000EvalBoard;
  // Open Opal Kelly XEM6010 board.
  errorCode = evalBoard->open();
  if (errorCode < 1) 
    {
      if (errorCode == -1) 
  	{
  	  fprintf(stderr,"Cannot load Opal Kelly FrontPanel DLL\n");
  	  fprintf(stderr,"Opal Kelly USB drivers not installed.\n ");
  	} 
      else 
  	{
  	  fprintf(stderr,"Intan RHD2000 USB Interface Board Not Found\n");
  	}
      delete evalBoard;
      evalBoard = 0;
      return;
    }
  
  // Load Rhythm FPGA configuration bitfile (provided by Intan Technologies).
  string bitfilename ="main.bit";
  if (!evalBoard->uploadFpgaBitfile(bitfilename)) {
    fprintf(stderr,"FPGA Configuration File Upload Error\n");
    fprintf(stderr,"Cannot upload configuration file to FPGA.  Make sure file main.bit is in the same directory as the executable file.\n");
    return;
  }
  
  /*
    // Initialize interface board.
    evalBoard->initialize();

    // Set sample rate and upload all auxiliary SPI command sequences.
    changeSampleRate(sampleRateComboBox->currentIndex());

    // Select RAM Bank 0 for AuxCmd3 initially, so the ADC is calibrated.
    evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortA, Rhd2000EvalBoard::AuxCmd3, 0);
    evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortB, Rhd2000EvalBoard::AuxCmd3, 0);
    evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortC, Rhd2000EvalBoard::AuxCmd3, 0);
    evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortD, Rhd2000EvalBoard::AuxCmd3, 0);

    // Since our longest command sequence is 60 commands, we run the SPI
    // interface for 60 samples.
    evalBoard->setMaxTimeStep(60);
    evalBoard->setContinuousRunMode(false);

    // Start SPI interface.
    evalBoard->run();

    // Wait for the 60-sample run to complete.
    while (evalBoard->isRunning()) {
        qApp->processEvents();
    }

    // Read the resulting single data block from the USB interface.
    Rhd2000DataBlock *dataBlock = new Rhd2000DataBlock(evalBoard->getNumEnabledDataStreams());
    evalBoard->readDataBlock(dataBlock);

    // We don't need to do anything with this data block; it was used to configure
    // the RHD2000 amplifier chips and to run ADC calibration.
    delete dataBlock;

    // Now that ADC calibration has been performed, we switch to the command sequence
    // that does not execute ADC calibration.
    evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortA, Rhd2000EvalBoard::AuxCmd3,
                                    fastSettleEnabled ? 2 : 1);
    evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortB, Rhd2000EvalBoard::AuxCmd3,
                                    fastSettleEnabled ? 2 : 1);
    evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortC, Rhd2000EvalBoard::AuxCmd3,
                                    fastSettleEnabled ? 2 : 1);
    evalBoard->selectAuxCommandBank(Rhd2000EvalBoard::PortD, Rhd2000EvalBoard::AuxCmd3,
                                    fastSettleEnabled ? 2 : 1);

    // Set default configuration for all eight DACs on interface board.
    evalBoard->enableDac(0, false);
    evalBoard->enableDac(1, false);
    evalBoard->enableDac(2, false);
    evalBoard->enableDac(3, false);
    evalBoard->enableDac(4, false);
    evalBoard->enableDac(5, false);
    evalBoard->enableDac(6, false);
    evalBoard->enableDac(7, false);
    evalBoard->selectDacDataStream(0, 0);
    evalBoard->selectDacDataStream(1, 0);
    evalBoard->selectDacDataStream(2, 0);
    evalBoard->selectDacDataStream(3, 0);
    evalBoard->selectDacDataStream(4, 0);
    evalBoard->selectDacDataStream(5, 0);
    evalBoard->selectDacDataStream(6, 0);
    evalBoard->selectDacDataStream(7, 0);
    evalBoard->selectDacDataChannel(0, 0);
    evalBoard->selectDacDataChannel(1, 1);
    evalBoard->selectDacDataChannel(2, 0);
    evalBoard->selectDacDataChannel(3, 0);
    evalBoard->selectDacDataChannel(4, 0);
    evalBoard->selectDacDataChannel(5, 0);
    evalBoard->selectDacDataChannel(6, 0);
    evalBoard->selectDacDataChannel(7, 0);
    evalBoard->setDacManual(Rhd2000EvalBoard::DacManual1, 32768);
    evalBoard->setDacManual(Rhd2000EvalBoard::DacManual2, 32768);
    evalBoard->setDacGain(0);
    evalBoard->setAudioNoiseSuppress(0);
  */
}
