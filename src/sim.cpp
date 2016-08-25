/*
  Author: Martin Horvat
  Date: September 29, 2008
  Description: Simulator of dynamical systems on 2d torus with a GUI
  License: GNU Lesser General Public License (LGPL) version 3
*/

// my libs
#include "sim.h"

int main (int argc, char *argv[]) {
 
   /* init threads */  
  if( !g_thread_supported() ) g_thread_init (NULL);
  
  gdk_threads_init ();
  
  gdk_threads_enter ();

  gtk_init (&argc, &argv);
  
  GladeXML  *data_handler;


  data.const_table = 0;
  data.formula_selected = -1;
  data.pixmap = 0;
  data.stop_running = false;
  data.listening = false;
  data.running = false;
  data.gc = 0;
  /* 
    load the interface 
  */
  data_handler = glade_xml_new ("sim.glade", NULL, NULL);

  /* 
    references to all important widgets 
  */
  data.main_window = glade_xml_get_widget (data_handler, "main_window"),
  
  data.conf_frame = glade_xml_get_widget (data_handler, "conf_frame");
  data.combobox = glade_xml_get_widget (data_handler, "combobox");
  data.saveas_button = glade_xml_get_widget (data_handler, "saveas_button");
  data.new_button = glade_xml_get_widget (data_handler, "new_button");
  data.delete_button = glade_xml_get_widget (data_handler, "delete_button");

  
  data.formula_frame = glade_xml_get_widget (data_handler, "formula_frame");
  data.F_entry[0] = glade_xml_get_widget (data_handler, "F1_entry");
  data.F_entry[1] = glade_xml_get_widget (data_handler, "F2_entry");
  data.L_spinbutton[0] = glade_xml_get_widget (data_handler, "L1_spinbutton");
  data.L_spinbutton[1] = glade_xml_get_widget (data_handler, "L2_spinbutton");
  
  data.const_frame = glade_xml_get_widget (data_handler, "const_frame");
  data.const_scrolledwindow = glade_xml_get_widget (data_handler, "const_scrolledwindow");
  data.addempty_button = glade_xml_get_widget (data_handler, "addempty_button");

  data.color_table  = glade_xml_get_widget (data_handler, "color_table");
  
  data.start_button = glade_xml_get_widget (data_handler, "start_button");
  data.stop_button = glade_xml_get_widget (data_handler, "stop_button");
  data.clear_button = glade_xml_get_widget (data_handler, "clear_button");
  data.savepic_button = glade_xml_get_widget (data_handler, "savepic_button");
  
  data.sleep_hscale = glade_xml_get_widget (data_handler, "sleep_hscale");

  data.drawingarea = glade_xml_get_widget (data_handler, "drawingarea");
  data.x_label = glade_xml_get_widget (data_handler, "x_label");
  data.y_label = glade_xml_get_widget (data_handler, "y_label");
 
 
  data.saveas_dialog = glade_xml_get_widget (data_handler, "saveas_dialog");
  data.d_comboboxentry = glade_xml_get_widget (data_handler, "d_comboboxentry");
 
  /*
    callback functions
  */
  g_signal_connect (G_OBJECT (data.main_window), 
                    "delete_event",  G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (G_OBJECT (data.main_window), 
                    "destroy", G_CALLBACK (gtk_main_quit), NULL);

  g_signal_connect (G_OBJECT (data.combobox), 
                    "changed", G_CALLBACK (on_combobox_changed), NULL);   
  g_signal_connect (G_OBJECT (data.new_button), 
                    "clicked", G_CALLBACK (on_new_button_clicked), NULL);  
  g_signal_connect (G_OBJECT (data.addempty_button), 
                    "clicked", G_CALLBACK (on_addempty_button_clicked), NULL);    

  // adding another formula
  g_signal_connect (G_OBJECT (data.saveas_button), 
                    "clicked", G_CALLBACK (on_saveas_button_clicked), NULL);    
  g_signal_connect (G_OBJECT (data.delete_button), 
                    "clicked", G_CALLBACK (on_delete_button_clicked), NULL);    
 
  //sleep time  
  g_signal_connect (G_OBJECT (data.sleep_hscale), "change-value",
                    G_CALLBACK (on_sleep_hscale_change), NULL);


  g_signal_connect (G_OBJECT (data.start_button), 
                      "clicked", G_CALLBACK (start_button_clicked), NULL);
  g_signal_connect (G_OBJECT (data.stop_button), 
                      "clicked", G_CALLBACK (stop_button_clicked), NULL);
  g_signal_connect (G_OBJECT (data.clear_button), 
                      "clicked", G_CALLBACK (clear_button_clicked), NULL);
  g_signal_connect (G_OBJECT (data.savepic_button), 
                      "clicked", G_CALLBACK (savepic_button_clicked), NULL);

    // plotting and simulation 
  g_signal_connect (G_OBJECT (data.drawingarea), "expose-event", 
                    G_CALLBACK (on_expose_event), NULL);
  g_signal_connect (G_OBJECT (data.drawingarea), "button-press-event",
                    G_CALLBACK (button_press_event), NULL);
  g_signal_connect (G_OBJECT (data.drawingarea), "motion-notify-event",
                    G_CALLBACK (on_motion_event), NULL);
  g_signal_connect (G_OBJECT(data.drawingarea),"configure-event",
                    G_CALLBACK(on_configure_event), NULL);

  // saveas dialog
  g_signal_connect_swapped (data.saveas_dialog,"response", 
                            G_CALLBACK (gtk_widget_hide), data.saveas_dialog);

  /*
    other setups 
  */

  // loading formulas from sim.cfg
  load_formula(data.formulas);
  
  // setting the menu with formula's names
  Tvec_str names;
  get_formulas_names(data.formulas, names);     
    
  setup_combobox_formula(GTK_COMBO_BOX(data.combobox), names);

  // setting the color table
  setup_colors(GTK_TABLE(data.color_table));
 
   // unreferencing the datahandlet
  g_object_unref (data_handler);
 
   /* 
    start the event loop 
  */ 
  gtk_widget_show_all (data.main_window);
  gtk_main ();
  
  gdk_threads_leave ();
  
}
