#if !defined(__sim_h)
#define __sim_h

/*
  Author: Martin Horvat
  Date: September 29, 2008
  Description: Header for sim.cpp
*/

#include <iostream>
#include <vector>
#include <cmath>
#include <sstream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <set>
#include <ios>
#include <unistd.h>

// parser of mathematical expressions
#include <muParser.h>

// GUI related headers
#include <gtk/gtk.h>
#include <glade/glade.h>
#include <pthread.h>

#define Tvec_str std::vector<std::string> 
#define Tvec_f std::vector<double> 

// formally enablin G_THREADS
#define G_THREADS_ENABLED

/*
  Formulas and general data
*/

struct Tformula {
  std::string name;
  
  Tvec_str expression,  // the two formulas
           consts;      // names of constants
           
  double L[2]; // modulus
};

struct Tdata {

  GtkWidget *main_window,
            // formula
            *formula_frame,
            *F_entry[2], *L_spinbutton[2],
            *saveas_button,
            *delete_button,
            *new_button,
            // constants
            *const_frame,
            *const_scrolledwindow,
            *const_table,
            *addempty_button,
            // selecting formulas
            *conf_frame,
            *combobox,
            // colors
            *color_togglebuttons[4][4],
            *color_table,
            // canvas & sumulation
            *sleep_hscale,
            *drawingarea,
            *start_button,
            *stop_button,
            *clear_button,
            *savepic_button,
            *x_label,
            *y_label,
            // save_as  dialog
            *saveas_dialog,
            *d_comboboxentry;

            
            
  // references to spinbuttons holding constants
  std::vector <GtkWidget*> const_value, 
                           const_name;
  
  int formula_selected;          
  std::vector<Tformula> formulas;

  // color palette
  int color_selected[2];   
  GdkColor colors[4][4];
  GdkGC *gc;

  // canvas
  GdkPixmap *pixmap;
  
  // flags for proper working of the threads
  bool running, 
       listening,
       stop_running;

  pthread_t thread_simulate;
  
} data;



/**********************************************************
  Mathematical functions
**********************************************************/

double mod(const double &u, const double &s){
 if (u > s) return u - s*int(u/s);
 if (u < 0.0) return u - s*(int(u/s)-1);
 return u;
}


/**********************************************************
  Managing formulas
**********************************************************/

/* Get constant out of expressions */
static double* constants_AddVariable(const char *a_szName, void *pUserData){

  double *p = new double;
  
  typedef std::pair< std::vector<double*>, std::set<std::string> > A;
  
  ((A *) pUserData)->first.push_back(p);
  
  ((A *) pUserData)->second.insert(std::string(a_szName));

  return p; 
}

bool get_constants(Tvec_str formulas, std::vector<std::string> & constants){

  double x[2];
 
  std::pair< std::vector<double*>, std::set<std::string> > list;
  try {
    for (int i = 0; i < 2; ++i) {

      if (formulas[i].size()){
        mu::Parser p;

        // standard variables
        p.DefineVar("x", x); 
        p.DefineVar("y", x + 1);     

        // standard constants
        p.DefineConst("pi", (double)M_PI);
        p.DefineConst("e", (double)M_E);

        p.SetVarFactory(constants_AddVariable, &list);
        p.SetExpr(formulas[i]);
        p.Eval();
      }
    }

  }

  catch (mu::Parser::exception_type &e){
    std::cerr << e.GetMsg() << '\n';
    return false;
  }

  typedef std::vector<double*> A;
  
  for (A::iterator it = list.first.begin(); it != list.first.end(); ++it) 
    delete (*it);
  
  constants.assign (list.second.begin(), list.second.end());
  return true;
}


void get_formulas_names(std::vector<Tformula>& formulas, 
                        std::vector<std::string> & names){
  names.clear();
  for (std::vector<Tformula>::iterator it = formulas.begin(); 
       it != formulas.end(); ++it) names.push_back((*it).name);
}

void load_formula(std::vector<Tformula> &formulas){
 
  formulas.clear();
  
  typedef std::vector<Tformula> A;

  std::ifstream in("sim.cfg");

  const int N = 1024;
  
  char s[N], *tok; 

  bool ok = true;
  
  while (in.getline(s, N) && ok) {

    Tformula f;

    //std::cerr << s << '\n';
    
    if ((tok = strtok(s, "|")) == 0) {ok = false; break;}
    f.name = std::string(tok);

    if ((tok = strtok(0, "|")) == 0) {ok = false; break;}
    f.expression.push_back(std::string(tok));

    if ((tok = strtok(0, "|")) == 0) {ok = false; break;}
    f.expression.push_back(std::string(tok));

    if ((tok = strtok(0, "|")) == 0) {ok = false; break;}    
    f.L[0] = atof(tok);

    if ((tok = strtok(0, "|")) == 0) {ok = false; break;}    
    f.L[1] = atof(tok);
    
    while ((tok = strtok(0, "|"))) 
      f.consts.push_back(std::string(tok));
      
    formulas.push_back(f);
  } 
  
  if (!ok) std::cerr << "There is something wrong with the database\n";
}

void save_formula(std::vector<Tformula> &formulas){

  std::ofstream os("sim.cfg", std::ios::trunc);
  
  for (std::vector<Tformula>::iterator it = formulas.begin(); 
       it != formulas.end(); ++it) {
       
    os << (*it).name << '|' 
       << (*it).expression[0] << '|' 
       << (*it).expression[1] << '|'
       << (*it).L[0] << '|' 
       << (*it).L[1];
       
    for (Tvec_str::iterator it_ = (*it).consts.begin(); 
         it_ != (*it).consts.end(); ++it_)
      os << '|' << (*it_);
      
    os << '\n';  
  }
}

void setup_combobox_formula(GtkComboBox *combo, Tvec_str &list){
  gtk_list_store_clear (GTK_LIST_STORE (gtk_combo_box_get_model (combo))); 

  for (Tvec_str::iterator it = list.begin(); it != list.end(); ++it)
    gtk_combo_box_append_text(GTK_COMBO_BOX(combo), (*it).c_str());
}

void setup_const(Tvec_str *consts);

void display_formula(Tformula *formula){

  if (formula) {
    for (int i = 0; i < 2; ++i) {
     gtk_entry_set_text(GTK_ENTRY(data.F_entry[i]), formula->expression[i].c_str());
     gtk_spin_button_set_value(GTK_SPIN_BUTTON(data.L_spinbutton[i]), formula->L[i]);
    }

    setup_const(&formula->consts);
  } else {

    for (int i = 0; i < 2; ++i) {
     gtk_entry_set_text(GTK_ENTRY(data.F_entry[i]),"");
     gtk_spin_button_set_value(GTK_SPIN_BUTTON(data.L_spinbutton[i]), 0);
    }
    setup_const(0);
  }
}

void on_combobox_changed(GtkWidget *widget, gpointer user_data){
  data.formula_selected = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
  
  if (data.formula_selected >= 0)
    display_formula(&data.formulas[data.formula_selected]);
  else 
    display_formula(0);
}

void on_new_button_clicked(GtkWidget *widget, gpointer user_data){
  data.formula_selected = -1;
  gtk_combo_box_set_active(GTK_COMBO_BOX(data.combobox),data.formula_selected);  
  display_formula(0);
}

void on_saveas_button_clicked(GtkWidget *widget, gpointer user_data){

  // creating a list of formulas in the saveas dialog combobox entry
  Tvec_str names; 
  get_formulas_names (data.formulas, names);
  setup_combobox_formula(GTK_COMBO_BOX(data.d_comboboxentry), names); 

  if (gtk_dialog_run (GTK_DIALOG (data.saveas_dialog)) == 0) {

    // getting the name of the formula
    gchar *name 
      = gtk_combo_box_get_active_text (GTK_COMBO_BOX(data.d_comboboxentry));

    // checking whether there such name exists   
    if (name && strlen(name) > 0) {
      int i;
      // creating the record for the formula
      Tformula  f;
      
      for (i = 0; i < 2; ++i) {
        f.expression.push_back(
          std::string(gtk_entry_get_text(GTK_ENTRY(data.F_entry[i])))
        );
        f.L[i] = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data.L_spinbutton[i]));  
      }

      if (!get_constants(f.expression, f.consts)) {
        GtkWidget *message = gtk_message_dialog_new (GTK_WINDOW(data.main_window),
          GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
          GTK_BUTTONS_CLOSE, "Something is not right with expressions!");
        gtk_dialog_run (GTK_DIALOG (message));
        gtk_widget_destroy (GTK_WIDGET(message)); 
        return; 
      }

      f.name.assign(name);

      for (i = 0; i < (int)data.formulas.size(); ++i)
        if (data.formulas[i].name.compare(f.name)== 0){
          data.formulas[i] = f;
          break;
        }
   
      if (i == (int)data.formulas.size()) data.formulas.push_back(f);      

      // saving the formulas to the file
      save_formula(data.formulas);

      // creating new list in combo_box
      names.clear();
      get_formulas_names (data.formulas, names);
      setup_combobox_formula(GTK_COMBO_BOX(data.combobox), names); 

      // selective active in the combo_box
      data.formula_selected = i;
      gtk_combo_box_set_active(GTK_COMBO_BOX(data.combobox),data.formula_selected); 

    } else {
      GtkWidget *message = gtk_message_dialog_new (GTK_WINDOW(data.main_window),
          GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
          GTK_BUTTONS_CLOSE, "Name of length 0 is not permitted!");
        gtk_dialog_run (GTK_DIALOG (message));
        gtk_widget_destroy (GTK_WIDGET(message)); 
    }
   if (name) g_free(name);
  }
  gtk_widget_hide(data.saveas_dialog); 
}

void on_delete_button_clicked(GtkWidget *widget, gpointer user_data){
  
  if (data.formula_selected >=0) {
    GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(data.main_window),
      GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION,
      GTK_BUTTONS_OK_CANCEL, 
      "Do you realy want to delete the formula \" %s \"?",
      data.formulas[data.formula_selected].name.c_str()
      );

    if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
      data.formulas.erase(data.formulas.begin() + data.formula_selected);
      save_formula(data.formulas);
      
      Tvec_str names; 
      get_formulas_names (data.formulas, names);
      setup_combobox_formula(GTK_COMBO_BOX(data.combobox), names); 
      data.formula_selected = -1;
      gtk_combo_box_set_active(GTK_COMBO_BOX(data.combobox),data.formula_selected); 
    }
    gtk_widget_destroy (dialog);
  }
 
}
/**********************************************************
  Constants
**********************************************************/

void setup_const(Tvec_str *consts){

  Tformula f;
    
  // clean the past
  if (data.const_table) {
    gtk_widget_destroy(data.const_table);
    data.const_table = 0;
  }  
  
  data.const_name.clear();
  data.const_value.clear();
 
  // start all over
  
  if (consts == 0) return;
   
  int M = consts->size();
  
  if (M > 0) {
    
    data.const_table = gtk_table_new(M, 3, FALSE);

    GtkWidget *g;
              
    for (int i = 0; i < M; ++i) {
      // creating the entry box
      g = gtk_entry_new_with_max_length(10);
      gtk_widget_set_size_request(g, 35,-1);
      
      gtk_entry_set_text(GTK_ENTRY(g), (*consts)[i].c_str());
      gtk_table_attach_defaults(GTK_TABLE(data.const_table), g, 0, 1, i, i+1);
      data.const_name.push_back(g);

      // creating the label "=" 
      g = gtk_label_new("=");
      gtk_table_attach_defaults(GTK_TABLE(data.const_table), g, 1, 2, i, i+1);
      
      // creating the spinbutton
      g = gtk_spin_button_new_with_range(-1e5, 1e5, 1);
      gtk_spin_button_set_digits(GTK_SPIN_BUTTON(g), 4);      
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(g), 0);
      
      // adding spinbutton to the table
      gtk_table_attach_defaults(GTK_TABLE(data.const_table), g , 2, 3, i, i+1); 
      
      // adding spinbutton value to the vector
      data.const_value.push_back(g);
    }
    // adding table to scroll_window
    gtk_scrolled_window_add_with_viewport(
      GTK_SCROLLED_WINDOW(data.const_scrolledwindow), data.const_table); 
      
    gtk_widget_show_all(data.const_table);
  }
}

void on_addempty_button_clicked(GtkWidget *widget, gpointer user_data){

  bool new_table = (data.const_table == 0);

  int M;
  
  if (new_table) 
    data.const_table = gtk_table_new((M = 1), 3, FALSE);
  else {
    g_object_get(G_OBJECT(data.const_table), "n-rows", &M, NULL);    
    ++M;
    gtk_table_resize(GTK_TABLE(data.const_table), M, 3);
  }

 
  GtkWidget *g;
          
  // creating the entry box
  g = gtk_entry_new_with_max_length(10);
  gtk_widget_set_size_request(g, 35,-1);
  gtk_entry_set_text(GTK_ENTRY(g), "");
  gtk_table_attach_defaults(GTK_TABLE(data.const_table), g, 0, 1, M-1, M);
  data.const_name.push_back(g);

  // creating the label "=" 
  g = gtk_label_new("=");
  gtk_table_attach_defaults(GTK_TABLE(data.const_table), g, 1, 2, M-1, M);

  // creating the spinbutton
  g = gtk_spin_button_new_with_range(-1e5, 1e5, 1);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(g), 4);      
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(g), 0);

  // adding spinbutton to the table
  gtk_table_attach_defaults(GTK_TABLE(data.const_table), g , 2, 3, M-1, M); 

  // adding spinbutton to the vector
  data.const_value.push_back(g);

  // adding table to scroll_window
  if (new_table) 
    gtk_scrolled_window_add_with_viewport(
      GTK_SCROLLED_WINDOW(data.const_scrolledwindow), data.const_table); 
      
  gtk_widget_show_all(data.const_table);
}

/**********************************************************
   Setting the colors
**********************************************************/


void on_color_toggled (GtkWidget *widget, gpointer user_data){

  int i = gtk_widget_get_name (widget)[2]-48, 
      j = gtk_widget_get_name (widget)[3]-48;
 
  if (data.color_selected[0] != i || data.color_selected[1] != j) {
    GtkWidget *g = 
    data.color_togglebuttons[data.color_selected[0]][data.color_selected[1]];
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(g), FALSE);  
    
    data.color_selected[0] = i;
    data.color_selected[1] = j;
  }
}


void setup_colors(GtkTable *parent) {

  GtkWidget *g;

  char s[10];
  
 /* const char *list[16] = {
    "white", "yellow", "fuchsia", "red", "silver", "gray", "olive", "purple",
    "maroon", "aqua", "lime", "teal", "green", "blue", "navy", "black"};
 */
   const char *list[16] = {  
  "#FFFFFF", "#FFFF00", "#FF00FF", "#FF0000", 
  "#C0C0C0", "#808080", "#808000", "#800080",
  "#800000", "#00FFFF", "#00FF00", "#008080", 
  "#008000", "#0000FF", "#000080", "#000000"};
  
  
  int i, j, k = 0;
  
  for (i = 0; i < 4; ++i)
    for (j = 0; j < 4; ++j, ++k) {
      data.color_togglebuttons[i][j] = g = gtk_toggle_button_new();
      
      // setting gtkwidget's name       
      sprintf (s,"tg%d%d", i, j); 
      gtk_widget_set_name(g, s);
      
      // setting the color of the widget
      gdk_color_parse (list[k], &data.colors[i][j]);
      gtk_widget_modify_bg(g, GTK_STATE_NORMAL, &data.colors[i][j]);
      gtk_widget_modify_bg(g, GTK_STATE_ACTIVE, &data.colors[i][j]);           
      
      // attaching  toggle_button to table
      gtk_table_attach_defaults(parent, g , j, j+1, i, i+1);
       
      g_signal_connect (G_OBJECT(g), "pressed", G_CALLBACK (on_color_toggled), 0);
    }


  data.color_selected[0] = data.color_selected[1] = 3;
  
  gtk_toggle_button_set_active(
    GTK_TOGGLE_BUTTON(data.color_togglebuttons[3][3]),TRUE);
}

/**********************************************************
  Plotting and simulation
**********************************************************/
void make_others_sensitive(bool sensitive){
  gtk_widget_set_sensitive (data.conf_frame, sensitive);
  gtk_widget_set_sensitive (data.formula_frame, sensitive);
  gtk_widget_set_sensitive (data.const_frame, sensitive);
}

/*
  Draw a point x,y in [0,1] rescaled on the canvas. 
  NOTE: The origin is top left, but mathematical origin should be bottom left.
*/

void draw(const double & x, const double &y){
  GdkRectangle update_rect;
  
  update_rect.x = data.drawingarea->allocation.width*x;
  update_rect.y = data.drawingarea->allocation.height*y;

  update_rect.width = 1;
  update_rect.height = 1;

  gdk_gc_set_foreground(data.gc, 
    &data.colors[data.color_selected[0]][data.color_selected[1]]);

  gdk_draw_rectangle (data.pixmap, data.gc, TRUE,
                      update_rect.x, update_rect.y,
                      update_rect.width, update_rect.height);

  gtk_widget_draw (data.drawingarea, &update_rect);
}

// formula of the dynamical system
Tformula formula;

// values of constants
std::vector<double> const_values;

static volatile struct Tsimulate {  
  // initial point
  double x[2];
  
  // checking if the initial point has changed
  bool point_changed;

  // sleep time
  int sleep_time;
} sim;

G_LOCK_DEFINE_STATIC (sim);

/*
  Simulation of the dynamical system
*/
void *simulate(void *args){

  int i, j;
  
  double x0[2], x1[2], L[2];

  try {
    L[0] = formula.L[0];
    L[1] = formula.L[1];

    mu::Parser p[2];
    for (i = 0; i < 2; ++i){
      p[i].DefineVar("x", x0); 
      p[i].DefineVar("y", x0 + 1); 
    }
      
    for (i = 0; i < 2; ++i) {
      p[i].DefineConst("pi", (double)M_PI);
      p[i].DefineConst("e", (double)M_E);
      
      for (j = 0; j < int(formula.consts.size()); ++j)
        p[i].DefineConst(formula.consts[j], const_values[j]);
      
      p[i].SetExpr(formula.expression[i]);
    }  

    while (!data.stop_running) {

      G_LOCK(sim);
      if (sim.point_changed){    
        x0[0] = sim.x[0];
        x0[1] = sim.x[1];
        sim.point_changed = false;
      }
      G_UNLOCK(sim);

      gdk_threads_enter ();
      draw(x0[0]/L[0], 1-x0[1]/L[1]);
      gdk_threads_leave ();
      
      for (i = 0; i < 2; ++i) x1[i] = p[i].Eval();
      for (i = 0; i < 2; ++i) x0[i] = mod(x1[i], L[i]);
      
      usleep(sim.sleep_time);
    }
  }

  catch (...) {
    gdk_threads_enter ();
    GtkWidget *message = gtk_message_dialog_new (GTK_WINDOW(data.main_window),
      GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
       GTK_BUTTONS_CLOSE, "There was an error in evalution of the map!");
    gtk_dialog_run (GTK_DIALOG (message));
    gtk_widget_destroy (GTK_WIDGET(message)); 
    gdk_threads_leave ();
      
    data.running = false;
    data.listening = false;
    make_others_sensitive(true);
    return NULL;
  }

  data.running = false;
  data.listening = false;
  make_others_sensitive(true);
  return NULL;
}

void start_button_clicked (GtkWidget *widget, gpointer user_data){
  
  int i,j;
  
  formula.expression.clear();
    
  for (i = 0; i < 2; ++i) {
    formula.expression.push_back(
      std::string(gtk_entry_get_text(GTK_ENTRY(data.F_entry[i]))));
    formula.L[i] 
      = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data.L_spinbutton[i]));  
  }
  
  if (formula.expression[0].size()+formula.expression[1].size()==0){
    GtkWidget *message = gtk_message_dialog_new (GTK_WINDOW(data.main_window),
      GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
       GTK_BUTTONS_CLOSE, "Map should be nontrivial!");
    gtk_dialog_run (GTK_DIALOG (message));
    gtk_widget_destroy (GTK_WIDGET(message));
    return;
  }

  formula.consts.clear();
  const_values.clear();
  
  // reading constants
  for (i = 0; i < (int)data.const_name.size(); ++i){
    const char *name = gtk_entry_get_text(GTK_ENTRY(data.const_name[i]));
    double value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data.const_value[i]));

    if (name && strlen(name)>0){
      formula.consts.push_back(std::string(name));
      const_values.push_back(value);
    } 
  }

  // checking the whether all needed constants are available
  Tvec_str consts_needed; 
  if (!get_constants(formula.expression, consts_needed)){
    GtkWidget *message = gtk_message_dialog_new (GTK_WINDOW(data.main_window),
      GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
      GTK_BUTTONS_CLOSE, "Something is not right with expressions!");
    gtk_dialog_run (GTK_DIALOG (message));
    gtk_widget_destroy (GTK_WIDGET(message)); 
    return;
  }

  std::string s;

  for (i = 0; i < (int)consts_needed.size(); ++i){
    s = consts_needed[i];

    for (j = 0; j < (int)formula.consts.size(); ++j) 
      if (formula.consts[j].compare(s)==0) break;

    if (j == (int) formula.consts.size()) {

      s = consts_needed[0];

      for (j = 1; j < (int)consts_needed.size()-1; ++j)
        s += "," + std::string(consts_needed[j]);
        
      GtkWidget *message = gtk_message_dialog_new (GTK_WINDOW(data.main_window),
        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
         GTK_BUTTONS_CLOSE, "Model needs following constants: %s", s.c_str());
      gtk_dialog_run (GTK_DIALOG (message));
      gtk_widget_destroy (GTK_WIDGET(message));
      return;
    }
  }

  /*
  for (i = 0; i < formula.consts.size(); ++i)
    std::cerr << formula.consts[i] << '\t' << const_values[i] << '\n';
  */
 
  data.stop_running = false;
  data.listening = true;

  make_others_sensitive(false);
}

void stop_button_clicked (GtkWidget *widget, gpointer user_data){
  data.stop_running = true; 
  make_others_sensitive(true);
}

void clear_button_clicked (GtkWidget *widget, gpointer user_data){
  GdkRectangle update_rect;
  
  update_rect.x = 0;
  update_rect.y = 0;

  update_rect.width = data.drawingarea->allocation.width;
  update_rect.height = data.drawingarea->allocation.height;

  gdk_draw_rectangle (data.pixmap, data.drawingarea->style->white_gc, TRUE,
                      update_rect.x, update_rect.y,
                      update_rect.width, update_rect.height);

  gtk_widget_draw (data.drawingarea, &update_rect);  
}

void savepic_button_clicked (GtkWidget *widget, gpointer user_data){
  
  if (data.pixmap){
    // creating saveas dialog
    GtkWidget *dialog = gtk_file_chooser_dialog_new ("Save File",
     				      GTK_WINDOW(data.main_window),
     				      GTK_FILE_CHOOSER_ACTION_SAVE,
     				      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
     				      GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
     				      NULL);

    // creating filters for image formats
    GtkFileFilter *filter = gtk_file_filter_new(),
                  *allfilter = gtk_file_filter_new();

    gtk_file_filter_set_name(filter, "Images(*.bmp; *.jpg; *.png)");
    gtk_file_filter_add_pattern(filter, "*.jpg");  
    gtk_file_filter_add_mime_type(filter, "image/jpeg"); 

    gtk_file_filter_add_pattern(filter, "*.bmp");
 	  gtk_file_filter_add_mime_type(filter, "image/bmp");

    gtk_file_filter_add_pattern(filter, "*.png");  
 	  gtk_file_filter_add_mime_type(filter, "image/png"); 

    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog), filter);

    gtk_file_filter_set_name(allfilter, "All Files");
    gtk_file_filter_add_pattern(allfilter, "*"); 
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), allfilter); 

    // running saveas dialog
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
      char *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));

      GdkPixbuf *pixbuf = gdk_pixbuf_get_from_drawable(NULL,
        data.drawingarea->window, gtk_widget_get_colormap(data.drawingarea), 
        0, 0, 0, 0,
        data.drawingarea->allocation.width, data.drawingarea->allocation.height);
        
      int len = strlen(filename);
      gboolean status = true;

      if (len>4 && strcmp(filename+len-4, ".bmp")==0)
        status = gdk_pixbuf_save (pixbuf, filename, "bmp", NULL, NULL, NULL); 
      else if (len>4 && strcmp(filename+len-4, ".jpg")==0)
        status = gdk_pixbuf_save (pixbuf, filename, "jpeg", NULL, NULL, NULL); 
      else if (len>5 && strcmp(filename+len-5, ".jpeg")==0)
        status = gdk_pixbuf_save (pixbuf, filename, "jpeg", NULL, NULL, NULL); 
      else if (len>4 && strcmp(filename+len-4, ".png")==0)
        status = gdk_pixbuf_save (pixbuf, filename, "png", NULL, NULL, NULL); 
      else {
        GtkWidget *message = gtk_message_dialog_new (GTK_WINDOW(data.main_window),
          GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
          GTK_BUTTONS_CLOSE, "This filetype is not supported, file '%s'",
        filename);
        gtk_dialog_run (GTK_DIALOG (message));
        gtk_widget_destroy (GTK_WIDGET(message));
      }

      if (!status){
        GtkWidget *message = gtk_message_dialog_new (GTK_WINDOW(data.main_window),
          GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
          GTK_BUTTONS_CLOSE, "Grabbing of the picture was not successful");
        gtk_dialog_run (GTK_DIALOG (message));
        gtk_widget_destroy (GTK_WIDGET(message)); 
      }

      g_object_unref(pixbuf);
      
      //std::cerr << filename << '\n';
      g_free (filename);
    }
     
    gtk_widget_destroy (dialog);
  }
}

gboolean on_expose_event (GtkWidget *widget, 
                          GdkEventExpose *event, gpointer user_data) {
                          
  gdk_draw_pixmap(widget->window,
                  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
                  data.pixmap,
                  event->area.x, event->area.y,
                  event->area.x, event->area.y,
                  event->area.width, event->area.height);

  return FALSE;
}


/* Create a new backing pixmap of the appropriate size */
gboolean on_configure_event(GtkWidget *widget,GdkEventConfigure *event){

  if (data.gc == 0) {
    GdkColormap *colormap = gtk_widget_get_colormap(widget);
    data.gc = gdk_gc_new(widget->window);

    int i, j;
    
    for (i = 0; i < 4; ++i) 
      for (j = 0; j < 4; ++j)
        gdk_colormap_alloc_color(colormap, &data.colors[i][j], FALSE, TRUE);      
  }
     
  if (data.pixmap) gdk_pixmap_unref(data.pixmap);

  data.pixmap = gdk_pixmap_new(widget->window,
                          widget->allocation.width,
                          widget->allocation.height,
                          -1);

  gdk_draw_rectangle (data.pixmap, widget->style->white_gc, TRUE,
                      0, 0,
                      widget->allocation.width,
                      widget->allocation.height);

  return TRUE;
}

gboolean button_press_event( GtkWidget *widget, GdkEventButton *event ){
  
  if (data.listening && event->button == 1 && data.pixmap != NULL){
    G_LOCK(sim);
    sim.point_changed = true;
    sim.x[0] = formula.L[0]*double(event->x)/widget->allocation.width;
    sim.x[1] = formula.L[1]*(1-double(event->y)/widget->allocation.height);
    G_UNLOCK(sim);

    if (!data.running) {
      data.running = true;
      pthread_create (&data.thread_simulate, NULL, simulate, NULL);
    }
  }

  
  return TRUE;
}

gboolean on_motion_event( GtkWidget *widget, GdkEventButton *event ){
  char s[255];
  
  sprintf(s,"x=%lf", double(event->x)/widget->allocation.width);
  gtk_label_set_text(GTK_LABEL(data.x_label), s);
  
  sprintf(s,"y=%lf", (1-double(event->y)/widget->allocation.height));
  gtk_label_set_text(GTK_LABEL(data.y_label), s);

  return TRUE;
}

void on_sleep_hscale_change (GtkWidget *widget, gpointer user_data) {
  G_LOCK(sim);
  sim.sleep_time = int(gtk_range_get_value(GTK_RANGE(widget)));
  G_UNLOCK(sim);
}


#endif
