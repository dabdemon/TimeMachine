//Declare and Import references
#include "pebble.h"
#include "pebble_fonts.h"


	/*
#define MY_UUID { 0xA6, 0x90, 0xDB, 0xBE, 0x3F, 0x20, 0x49, 0xE5, 0x89, 0x7E, 0xFC, 0x66, 0xC6, 0x79, 0x2A, 0x7F }
PBL_APP_INFO(MY_UUID,
             "Time Machine", "dabdemon",
             1, 0, // App version //
             DEFAULT_MENU_ICON,
             APP_INFO_STANDARD_APP);

*/

#define WEEKDAY_FRAME	  (GRect(5,  2, 95, 168-145)) 
#define BATT_FRAME 	      (GRect(98,  4, 40, 168-146)) 
#define BT_FRAME 	      (GRect(125,  4, 25, 168-146))  
#define TIME_FRAME        (GRect(0, 15, 144, 168-16)) 
#define DATE_FRAME        (GRect(1, 69, 139, 168-62)) 

#define TIMER_FRAME		(GRect(0, 120, 144, 168-62))
#define TIMER_LABEL		(GRect(0, 95, 144, 168-148))

	
//******************//
// DEFINE THE ICONS //
//******************//	
static int LAYER_ICONS[] = {
	RESOURCE_ID_BT_CONNECTED,
	RESOURCE_ID_BT_DISCONNECTED,
};


//Declare initial window	
	Window *my_window;    

//Define the layers
	TextLayer *date_layer;   		// Layer for the date
	TextLayer *Time_Layer; 			// Layer for the time
	TextLayer *Weekday_Layer; 		//Layer for the weekday
	TextLayer *Batt_Layer;			//Layer for the BT connection
	TextLayer *BT_Layer;			//Layer for the BT connection
	TextLayer *count_down;			//Layer for the timer/chrono
	TextLayer *label_layer;			//Layer for the label
	
	bool setting_blink_state = true; 

	static GBitmap *BT_image;
	static BitmapLayer *BT_icon_layer; //Layer for the BT connection
	
	static GBitmap *Batt_image;
	static BitmapLayer *Batt_icon_layer; //Layer for the Battery status

//Define and initialize Date and Time variables
	//FONTS
	GFont font_date;        // Font for date
	GFont font_time;        // Font for time
	GFont font_update;      // Font for last update
	GFont font_timer;	// Font for the timer

	//Vibe Control
	bool BTConnected = true;

	//Date & Time	
	static char last_update[]="00:00 ";
	static int initial_minute;

	static char weekday_text[] = "XXXXXXXXXX";
	static char date_text[] = "XXX 00";
	static char month_text[] = "XXXXXXXXXXXXX";
	static char day_text[] = "31";
	static char day_month[]= "31 SEPTEMBER"; 
	static char time_text[] = "00:00"; 
	
	bool translate_sp = true;

//*****************//
// Timer Logic //
//*****************//
	bool chrono = false;

	enum State {
	  DONE,
	  SETTING,
	  PAUSED,
	  COUNTING_UP,
	  COUNTING_DOWN
	};

	enum State current_state = DONE;
	
	int chrono_init = 0;
	int total_seconds = 5 * 60;
	int current_seconds = 5 * 60;
	int last_set_time = -1;
	
	const VibePattern alarm_finished = {
	  .durations = (uint32_t []) {300, 150, 150, 150,  300, 150, 300},
	  .num_segments = 7
	};

// Setting state

	enum SettingUnit {
	  SETTING_SECOND = 2,
	  SETTING_MINUTE = 1,
	  SETTING_HOUR = 0,
	};

	enum SettingUnit setting_unit = SETTING_MINUTE;
	
	//Updates the countdown parameters based on user's input
	void update_countdown() {
	  if (current_seconds == last_set_time) {
		return;
	  }
	
	  static char time_text[] = " 00:00:00";
	
			  time_t now = time(NULL);
			  struct tm *stime = localtime(&now);
			  stime->tm_hour = current_seconds / (60 * 60);
			  stime->tm_min  = (current_seconds - stime->tm_hour * 60 * 60) / 60;
			  stime->tm_sec  = current_seconds - stime->tm_hour * 60 * 60 - stime->tm_min * 60;
			
			  strftime(time_text, sizeof(time_text), " %T", stime);
				
			  text_layer_set_text(count_down, time_text);
			  last_set_time = current_seconds;
	
	}

	void draw_setting_unit() {
	  //layer_mark_dirty(unit_marker);  
		if (setting_unit == SETTING_MINUTE){
				if (translate_sp){text_layer_set_text(label_layer, " Establecer Min");}
				else {text_layer_set_text(label_layer, " Set Minutes");}
		}
		else if (setting_unit == SETTING_SECOND){
				if (translate_sp){text_layer_set_text(label_layer, " Establecer Seg");}
				else {text_layer_set_text(label_layer, " Set Seconds");}
		}
		else if (setting_unit == SETTING_HOUR){
				if (translate_sp){text_layer_set_text(label_layer, " Establecer Horas");}
				else {text_layer_set_text(label_layer, " Set Hours");}
		}
	}

	void toggle_setting_mode(ClickRecognizerRef recognizer, void *context) {
	  if (current_state == SETTING) {
		current_state = DONE;
		if (translate_sp){text_layer_set_text(label_layer, " Temporizador");}
		else {text_layer_set_text(label_layer, " Timer");}
	  }
	  else {
		  if (chrono == false){ //TIMER CONTROL
				current_seconds = total_seconds;
				update_countdown();
				current_state = SETTING;
				setting_unit = SETTING_MINUTE;
				draw_setting_unit();
		  }
	  }
	}//toggle_setting_mode END

/*
	void unit_marker_update_callback(Layer *me, GContext* ctx) {
	  (void)me;
	
	  int width = 32;
	  int start = 8 + (width + 14) * setting_unit;
	
	  if (current_state == SETTING && setting_blink_state) {
		graphics_context_set_stroke_color(ctx, GColorWhite);
	
		for (int i = 0; i < 4; i++) {
		  graphics_draw_line(ctx, GPoint(start, 160 + i), GPoint(start + width, 160 + i)); //130 was 95
		}
	  }
	} //unit_marker_update_callback END
*/

	void select_pressed(ClickRecognizerRef recognizer, void *context) {
	//TIMER CONTROL
		if (chrono == false)
		{	
			  if (current_state == SETTING) {
				setting_unit = (setting_unit + 1) % 3;
				setting_blink_state = true;
				draw_setting_unit();
			  }
			  else if (current_state == PAUSED || current_state == DONE) {
				current_state = COUNTING_DOWN;
			  }
			  else {
				current_state = PAUSED;
			  }
		}
	//CHRONO CONTROL
		else
		{
			 if (current_state == PAUSED || current_state == DONE) 
			 {
				current_state = COUNTING_UP;
			 }
			  else {
				current_state = PAUSED;
			  }
		}
	}//select_pressed END

	void select_long_release_handler(ClickRecognizerRef recognizer, void *context) {
	  // This is needed to avoid missing clicks. Seems to be a bug in the SDK.
	}

	void increment_time(int direction) {
	  if (current_state == SETTING) {
		switch (setting_unit) {
		case SETTING_HOUR: direction *= 60;
		case SETTING_MINUTE: direction *= 60;
		default: break;
		}
	
		int new_seconds = total_seconds + direction;
		if (new_seconds >= 0 && new_seconds < 100 * 60 * 60) {
		  total_seconds = new_seconds;
		  current_seconds = total_seconds;
		  update_countdown();
		}
	  }
	}//increment_time END


	void init_timer(){ //Initialize the Timer/Chrono
		//TIMER CONTROL
		if (chrono==false){
			  if (current_state != SETTING) 
			  {
				current_state = DONE;
				current_seconds = total_seconds;
				update_countdown();
			  }
		} //CHRONO CONTROL
		else
		{
			current_state = DONE;
			current_seconds = chrono_init;
			update_countdown();
		}
		return;
	}//init_timer END

	void reset_timer(ClickRecognizerRef recognizer, void *context) {
		//reset the timer/chrono when double clicks the select button
		init_timer();
	}

	void button_pressed_up(ClickRecognizerRef recognizer, void *context) {
		// Switch between timer and chrono
		if (current_state == SETTING)
		{ 
			increment_time(1);
		}
		else 
		{
			if (translate_sp){text_layer_set_text(label_layer, " Cronometro");}
				else {text_layer_set_text(label_layer, " Chronograph");}
			chrono = true;
			init_timer();
		}
	} //button_pressed_up END

	void button_pressed_down(ClickRecognizerRef recognizer, void *context) {
		// Switch between timer and chrono
		if (current_state == SETTING)
		{ 
			increment_time(-1);
		}
		else
		{
			if (translate_sp){text_layer_set_text(label_layer, " Temporizador");}
				else {text_layer_set_text(label_layer, " Timer");}
			chrono = false;
			init_timer();
		}
	} //button_pressed_down END

	void handle_second_counting_up() {
	  current_seconds++;
	  update_countdown();
	}
	
	void handle_second_counting_down() {
	  current_seconds--;
	  update_countdown();
	
	  if (current_seconds == 0) {
		vibes_enqueue_custom_pattern(alarm_finished);
		current_state = DONE;
	  }
	}
	
	void handle_second_waiting() {
		//TIMER CONTROL
		if(chrono==false){
			current_seconds = total_seconds;
		}
		//CHRONO CONTROL
		else{
			current_seconds = chrono_init;
		}
	  update_countdown();
	}
	
	void handle_second_setting() {
	  //setting_blink_state = !setting_blink_state;
	  //layer_mark_dirty(unit_marker);
	}

// Time Logic END //

	//****************************//
	// Configure Buttons behavior //
	//****************************//

	void config_provider(Window *window) {
	 // SELECT BUTTON
	  window_single_click_subscribe(BUTTON_ID_SELECT, select_pressed);
	  window_long_click_subscribe(BUTTON_ID_SELECT,700,toggle_setting_mode,select_long_release_handler);
	  window_multi_click_subscribe(BUTTON_ID_SELECT,2,2,0,false,reset_timer);	
	  
	// UP BUTTON
	  window_single_click_subscribe(BUTTON_ID_UP, button_pressed_up);
		
	// DOWN BUTTON
	  window_single_click_subscribe(BUTTON_ID_DOWN, button_pressed_down);
	  
	} //config_provider END




//**************************//
// Check the Battery Status //
//**************************//

static void handle_battery(BatteryChargeState charge_state) {
  	static char battery_text[] = "100%";

  if (charge_state.is_charging) {
    //snprintf(battery_text, sizeof(battery_text), "charging");
	  Batt_image = gbitmap_create_with_resource(RESOURCE_ID_BATT_CHAR);
	  bitmap_layer_set_bitmap(Batt_icon_layer, Batt_image);
  } else {
	  //Kill previous image to don't display a wrong one
	  bitmap_layer_set_bitmap(Batt_icon_layer, NULL);
    //snprintf(battery_text, sizeof(battery_text), "%d%%", charge_state.charge_percent);
	  //if ((battery_text[0] == "1" || battery_text[0] == "2")  && strlen(battery_text)<4) //If the charge is between 0% and 20%
	  if (charge_state.charge_percent<10) //If the charge is between 0% and 10%
	  {
		Batt_image = gbitmap_create_with_resource(RESOURCE_ID_BATT_EMPTY);
	  	bitmap_layer_set_bitmap(Batt_icon_layer, Batt_image);
  }
  //text_layer_set_text(Batt_Layer, battery_text);
}
}

//******************************//
// Handle Bluetooth Connection  //
//*****************************//
static void handle_bluetooth(bool connected) 
{
  	//text_layer_set_text(BT_Layer, connected ? "C" : "D");
	
	//draw the BT icon if connected
	
	if(connected ==true)
	{
		BT_image = gbitmap_create_with_resource(RESOURCE_ID_BT_CONNECTED);
  		bitmap_layer_set_bitmap(BT_icon_layer, BT_image);
		if (BTConnected == false){
			//Vibes to alert connection
			vibes_double_pulse();
			BTConnected = true;
		}
	}
	else
	{
		//BT_image = gbitmap_create_with_resource(RESOURCE_ID_BT_DISCONNECTED);
  		bitmap_layer_set_bitmap(BT_icon_layer, NULL);
		if (BTConnected == true){
			//Vibes to alert disconnection
			vibes_long_pulse();
			BTConnected = false;
		}
	
	}
	
	
} //handle_bluetooth


void TranslateDate(){
			
			if (month_text[0] == 'J' && month_text[1] == 'a')
			{
				memcpy(&month_text, "   enero", strlen("   enero")+1); // January
			}
			
			if (month_text[0] == 'F' && month_text[1] == 'e')
			{
				memcpy(&month_text, "   febrero", strlen("   febrero")+1); // Febrary
			}
			
			if (month_text[0] == 'M' && month_text[2] == 'r')
			{
				memcpy(&month_text, "   marzo", strlen("   marzo")+1); // March
			}
			
			if (month_text[0] == 'A' && month_text[1] == 'p')
			{
				memcpy(&month_text, "   abril", strlen("   abril")+1); // April
			}
			
			if (month_text[0] == 'M' && month_text[2] == 'y')
			{
				memcpy(&month_text, "   de mayo", strlen("   de mayo")+1); // May
			}
			
			if (month_text[0] == 'J' && month_text[2] == 'n')
			{
				memcpy(&month_text, "   junio", strlen("   junio")+1); // June
			}
			
			if (month_text[0] == 'J' && month_text[2] == 'l')
			{
				memcpy(&month_text, "   julio", strlen("   julio")+1); // July
			}
			
			if (month_text[0] == 'A' && month_text[1] == 'u')
			{
				memcpy(&month_text, "   agosto ", strlen("   agosto ")+1); // August
			}
			
			if (month_text[0] == 'S' && month_text[1] == 'e')
			{
				memcpy(&month_text, "   septiembre", strlen("   septiembre")+1); // September
			}
			
			if (month_text[0] == 'O' && month_text[1] == 'c')
			{
				memcpy(&month_text, "   octubre", strlen("   octubre")+1); // October
			}
			
			if (month_text[0] == 'N' && month_text[1] == 'o')
			{
				memcpy(&month_text, "   noviembre", strlen("   noviembre")+1); // November
			}
			
			if (month_text[0] == 'D' && month_text[1] == 'e')
			{
				memcpy(&month_text, "   diciembre", strlen("   diciembre")+1); // December
			}
			
			// Primitive hack to translate the day of week to another language
			// Needs to be exactly 3 characters, e.g. "Mon" or "Mo "
			// Supported characters: A-Z, a-z, 0-9
			
			if (weekday_text[0] == 'M')
			{
				memcpy(&weekday_text, " Lunes", strlen(" Lunes")+1); // Monday
			}
			
			if (weekday_text[0] == 'T' && weekday_text[1] == 'u')
			{
				memcpy(&weekday_text, " Martes", strlen(" Martes")+1); // Tuesday
			}
			
			if (weekday_text[0] == 'W')
			{
				memcpy(&weekday_text, " Miercoles", strlen(" Miercoles")+1); // Wednesday
			}
			
			if (weekday_text[0] == 'T' && weekday_text[1] == 'h')
			{
				memcpy(&weekday_text, " Jueves", strlen(" Jueves")+1); // Thursday
			}
			
			if (weekday_text[0] == 'F')
			{
				memcpy(&weekday_text, " Viernes", strlen(" Viernes")+1); // Friday
			}
			
			if (weekday_text[0] == 'S' && weekday_text[1] == 'a')
			{
				memcpy(&weekday_text, " Sabado", strlen(" Sabado")+1); // Saturday
			}
			
			if (weekday_text[0] == 'S' && weekday_text[1] == 'u')
			{
				memcpy(&weekday_text, " Domingo", strlen(" Domingo")+1); // Sunday
			}
			

}



//************************//
// Capture the Tick event //
//************************//
void handle_tick(struct tm *tick_time, TimeUnits units_changed)
{

	//time_t lnow = time(NULL);
	struct tm local_time;
	
	//Check the Timer/Chrono state every second
	  switch(current_state) {
	  case DONE:
		handle_second_waiting();
		break;
	  case COUNTING_DOWN:
		handle_second_counting_down();
		break;
	  case COUNTING_UP:
		handle_second_counting_up();
		break;  
	  case SETTING:
		handle_second_setting();
		break;
	  default:
		break;
	  } //switch (current_state)
		
//Init the date
	
				//Get the Weekday
				strftime(weekday_text,sizeof(weekday_text),"%A",tick_time);
				//Get the Month + Day (English format)
				 strftime(month_text,sizeof(month_text),"%B %e",tick_time);
				//Get the Day + Month (Spanish format)
				strftime(day_month,sizeof(day_month),"%e %B",tick_time);


				if(translate_sp){
					//Get the Month
					strftime(month_text,sizeof(month_text),"%B",tick_time);
					//Get the day
					strftime(day_text,sizeof(day_text),"%e",tick_time);
					//Translate to Spanish
					TranslateDate();
					
					//Concatenate the day to the month
					memcpy(&month_text, day_text, strlen(day_text));
				}

						
				text_layer_set_text(date_layer, month_text);
				text_layer_set_text(Weekday_Layer, weekday_text); //Update the weekday layer	
				

	if (units_changed & MINUTE_UNIT) 
	{

			/*
			if (units_changed & DAY_UNIT)
			{	
			} // DAY CHANGES
			*/
		
			//Format the Local Time	
			if (clock_is_24h_style())
			{
				strftime(time_text, sizeof(time_text), "%H:%M", tick_time);
			}
			else
			{
				strftime(time_text, sizeof(time_text), "%I:%M", tick_time);
			}
		
	  			
  			text_layer_set_text(Time_Layer, time_text);

					
		
			//Check Battery Status
			handle_battery(battery_state_service_peek());
		
			//Check BT Status
			handle_bluetooth(bluetooth_connection_service_peek());

	} //MINUTE CHANGES
} //HANDLE_TICK 



//****************************//
// Initialize the application //
//****************************//

void handle_init(void)
{
	//Define Resources
    ResHandle res_d;
	ResHandle res_u;
	ResHandle res_t;
	ResHandle res_timer;
	
	//Create the main window
	my_window = window_create(); 
	window_set_fullscreen(my_window,true);
	window_stack_push(my_window, true /* Animated */);
	window_set_background_color(my_window, GColorBlack);

	
	//Load the custom fonts
	res_t = resource_get_handle(RESOURCE_ID_FUTURA_CONDENSED_53); // Time font
	res_d = resource_get_handle(RESOURCE_ID_FUTURA_17); // Date font
	res_u = resource_get_handle(RESOURCE_ID_FUTURA_14); // Last Update font
	res_timer =  resource_get_handle(RESOURCE_ID_FUTURA_30); //Timer/Chrono
	
		
    font_date = fonts_load_custom_font(res_d);
	font_update = fonts_load_custom_font(res_u);
	font_time = fonts_load_custom_font(res_t);
	font_timer = fonts_load_custom_font(res_timer);

	
	
	//LOAD THE LAYERS
		//Display the Weekday layer
		Weekday_Layer = text_layer_create(WEEKDAY_FRAME);
		text_layer_set_text_color(Weekday_Layer, GColorWhite);
		text_layer_set_background_color(Weekday_Layer, GColorClear);
		text_layer_set_font(Weekday_Layer, font_date);
		text_layer_set_text_alignment(Weekday_Layer, GTextAlignmentLeft);
		layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(Weekday_Layer)); 
	
		//Display the Batt layer
		Batt_icon_layer = bitmap_layer_create(BATT_FRAME);
  		bitmap_layer_set_bitmap(Batt_icon_layer, Batt_image);
  		layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(Batt_icon_layer));
	
		//Display the BT layer
	  	BT_icon_layer = bitmap_layer_create(BT_FRAME);
  		bitmap_layer_set_bitmap(BT_icon_layer, BT_image);
  		layer_add_child(window_get_root_layer(my_window), bitmap_layer_get_layer(BT_icon_layer));
	
		//Display the Time layer
		Time_Layer = text_layer_create(TIME_FRAME);
		text_layer_set_text_color(Time_Layer, GColorWhite);
		text_layer_set_background_color(Time_Layer, GColorClear);
		text_layer_set_font(Time_Layer, font_time);
		text_layer_set_text_alignment(Time_Layer, GTextAlignmentCenter);
		layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(Time_Layer)); 
	
		//Display the Date layer
		date_layer = text_layer_create(DATE_FRAME);
		text_layer_set_text_color(date_layer, GColorWhite);
		text_layer_set_background_color(date_layer, GColorClear);
		text_layer_set_font(date_layer, font_date);
		text_layer_set_text_alignment(date_layer, GTextAlignmentRight);
		layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(date_layer)); 
	
		// TIMER LABEL
		label_layer = text_layer_create(TIMER_LABEL);
		text_layer_set_text_color(label_layer, GColorBlack);
		text_layer_set_background_color(label_layer, GColorWhite);
		text_layer_set_font(label_layer, font_date);
		text_layer_set_text_alignment(label_layer, GTextAlignmentLeft);
		layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(label_layer)); 
		if (translate_sp){text_layer_set_text(label_layer, " Temporizador");}
				else {text_layer_set_text(label_layer, " Timer");}
			
		// TIMER
		count_down = text_layer_create(TIMER_FRAME);
		text_layer_set_text_color(count_down, GColorWhite);
		text_layer_set_background_color(count_down, GColorClear);
		text_layer_set_font(count_down, font_timer);
		text_layer_set_text_alignment(count_down, GTextAlignmentLeft);
		layer_add_child(window_get_root_layer(my_window), text_layer_get_layer(count_down)); 
	  	update_countdown();

	


	// Ensures time is displayed immediately (will break if NULL tick event accessed).
	  // (This is why it's a good idea to have a separate routine to do the update itself.)
	  	
		time_t now = time(NULL);
	  	struct tm *current_time = localtime(&now);
		handle_tick(current_time, MINUTE_UNIT);
		tick_timer_service_subscribe(SECOND_UNIT, &handle_tick);
	
		//Enable the Battery check event
		battery_state_service_subscribe(&handle_battery);
		//Enable the Bluetooth check event
	 	bluetooth_connection_service_subscribe(&handle_bluetooth);
	
		//*********//
		// BUTTONS //
		//*********//
		window_set_click_config_provider(my_window, (ClickConfigProvider) config_provider);
	
} //HANDLE_INIT



//**********************//
// Kill the application //
//**********************//
void handle_deinit(void)
{
  //text_layer_destroy(text_layer);

	//Unsuscribe services
	tick_timer_service_unsubscribe();
 	battery_state_service_unsubscribe();
  	bluetooth_connection_service_unsubscribe();
	
	if (BT_image) {gbitmap_destroy(BT_image);}
	if (Batt_image){gbitmap_destroy(Batt_image);}
	
	//Deallocate layers
	text_layer_destroy(Time_Layer);
	text_layer_destroy(date_layer);
	text_layer_destroy(Weekday_Layer);
	text_layer_destroy(label_layer);
	text_layer_destroy(count_down);
	
	//Deallocate custom fonts
	fonts_unload_custom_font(font_date);
	fonts_unload_custom_font(font_update);
	fonts_unload_custom_font(font_time);
	fonts_unload_custom_font(font_timer);
	
	//Deallocate the main window
  	window_destroy(my_window);

} //HANDLE_DEINIT


//*************//
// ENTRY POINT //
//*************//
int main(void) 
{	
	handle_init();
	app_event_loop();
	handle_deinit();
}
