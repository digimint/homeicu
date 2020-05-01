/* =========================================================
 * ====                   WARNING                        ===
 * =========================================================
 * The code in this tab has been generated from the GUI form
 * designer and care should be taken when editing this file.
 * Only add/edit code inside the event handlers i.e. only
 * use lines between the matching comment tags. e.g.

 void myBtnEvents(GButton button) { //_CODE_:button1:12356:
     // It is safe to enter your event code here  
 } //_CODE_:button1:12356:
 
 * Do not rename this tab!
 * =========================================================
 */

public void record_click(GButton source, GEvent event) { //_CODE_:record:731936:
  //println("record - GButton >> GEvent." + event + " @ " + millis());
  ////////////////////////////////////////////////////////////////////////////////
  //
  //    Enable the buttons and calls the serial port function
  //    Comselect is made true to call the serial function
  //  
  ///////////////////////////////////////////////////////////////////////////////
  try
  {
    jFileChooser = new JFileChooser();
    jFileChooser.setSelectedFile(new File("log.csv"));
    jFileChooser.showSaveDialog(null);
    String filePath = jFileChooser.getSelectedFile()+"";

    if ((filePath.equals("log.txt"))||(filePath.equals("null")))
    {
    } else
    {    
      done.setVisible(true);
      record.setVisible(false);
      close.setEnabled(false);
      close.setLocalColorScheme(GCScheme.CYAN_SCHEME);
      logging = true;
      date = new Date();
      output = new FileWriter(jFileChooser.getSelectedFile(), true);
      bufferedWriter = new BufferedWriter(output);
      bufferedWriter.write(date.toString()+"");
      bufferedWriter.newLine();
      bufferedWriter.write("TimeStramp,ECG,SpO2,Respiration");
      bufferedWriter.newLine();
    }
  }
  catch(Exception e)
  {
    println("File Not Found");
  }
} //_CODE_:record:731936:

public void done_click(GButton source, GEvent event) { //_CODE_:done:614170:
  //println("done - GButton >> GEvent." + event + " @ " + millis());
  ////////////////////////////////////////////////////////////////////////////////
  //
  //    Save the file and displays a success message
  //  
  ///////////////////////////////////////////////////////////////////////////////
  if (logging == true)
  {
    showMessageDialog(null, "Log File Saved successfully");

    record.setVisible(true);
    done.setVisible(false);

    close.setEnabled(true);
    close.setLocalColorScheme(GCScheme.GREEN_SCHEME);
    record.setEnabled(true);
    record.setLocalColorScheme(GCScheme.GREEN_SCHEME);
    done.setEnabled(true);
    done.setLocalColorScheme(GCScheme.GREEN_SCHEME);
    logging = false;
    try
    {
      bufferedWriter.flush();
      bufferedWriter.close();
    }
    catch(Exception e)
    {
      println(e);
    }
  }
} //_CODE_:done:614170:

public void close_click(GButton source, GEvent event) { //_CODE_:close:222350:
  //println("close - GButton >> GEvent." + event + " @ " + millis());
  int dialogResult = JOptionPane.showConfirmDialog (null, "Would You Like to Close The Application?");
  if (dialogResult == JOptionPane.YES_OPTION) {
    try
    {
      Runtime runtime = Runtime.getRuntime();
      Process proc = runtime.exec("sudo shutdown -h now");
      System.exit(0);
    }
    catch(Exception e)
    {
      exit();
    }
  } else
  {
  }
} //_CODE_:close:222350:

public void imgButton1_click1(GImageButton source, GEvent event) { //_CODE_:imgButton1:665258:
  println("imgButton1 - GImageButton >> GEvent." + event + " @ " + millis());
} //_CODE_:imgButton1:665258:

public void grid_size_click(GButton source, GEvent event) { //_CODE_:grid_size:708711:
  println("grid_size - GButton >> GEvent." + event + " @ " + millis());
} //_CODE_:grid_size:708711:

public void gridStatus_click(GButton source, GEvent event) { //_CODE_:gridStatus:949626:
  //println("gridStatus - GButton >> GEvent." + event + " @ " + millis());
  if (gStatus)
  {
    gStatus = false;
    la.setText("Grid :OFF");
  } else
  {
    gStatus = true;    
    la.setText("Grid :   ON");
  }
  la.setTextBold();
} //_CODE_:gridStatus:949626:



// Create all the GUI controls. 
// autogenerated do not edit
public void createGUI(){
  G4P.messagesEnabled(false);
  G4P.setGlobalColorScheme(GCScheme.BLUE_SCHEME);
  G4P.setCursor(ARROW);
  surface.setTitle("Healthy Pi");
  record = new GButton(this, 445, 420, 100, 55);
  record.setText("RECORD");
  record.setTextBold();
  record.setLocalColorScheme(GCScheme.GREEN_SCHEME);
  record.addEventHandler(this, "record_click");
  done = new GButton(this, 445, 420, 100, 55);
  done.setText("DONE");
  done.setTextBold();
  done.setLocalColorScheme(GCScheme.GREEN_SCHEME);
  done.addEventHandler(this, "done_click");
  close = new GButton(this, 560, 420, 100, 55);
  close.setText("CLOSE");
  close.setTextBold();
  close.setLocalColorScheme(GCScheme.GREEN_SCHEME);
  close.addEventHandler(this, "close_click");
  imgButton1 = new GImageButton(this, 694, 420, 100, 55, new String[] { "logo.jpg", "logo.jpg", "logo.jpg" } );
  imgButton1.addEventHandler(this, "imgButton1_click1");
  bpm1 = new GLabel(this, 620, 15, 139, 100);
  bpm1.setTextAlign(GAlign.CENTER, GAlign.MIDDLE);
  bpm1.setText("---");
  bpm1.setTextBold();
  bpm1.setOpaque(false);
  SP02 = new GLabel(this, 620, 115, 139, 100);
  SP02.setTextAlign(GAlign.CENTER, GAlign.MIDDLE);
  SP02.setText("---");
  SP02.setTextBold();
  SP02.setOpaque(false);
  BP = new GLabel(this, 590, 215, 225, 100);
  BP.setTextAlign(GAlign.CENTER, GAlign.MIDDLE);
  BP.setText("---/---");
  BP.setTextBold();
  BP.setOpaque(false);
  label1 = new GLabel(this, 600, 210, 75, 20);
  label1.setText("BP");
  label1.setTextBold();
  label1.setOpaque(false);
  label2 = new GLabel(this, 600, 105, 150, 20);
  label2.setText("SpO2            %");
  label2.setTextBold();
  label2.setOpaque(false);
  label3 = new GLabel(this, 600, 5, 80, 20);
  label3.setText("HR");
  label3.setTextBold();
  label3.setOpaque(false);
  label4 = new GLabel(this, 720, 210, 80, 20);
  label4.setTextAlign(GAlign.RIGHT, GAlign.MIDDLE);
  label4.setText("mmHg");
  label4.setTextBold();
  label4.setOpaque(false);
  label6 = new GLabel(this, 720, 5, 80, 20);
  label6.setTextAlign(GAlign.RIGHT, GAlign.MIDDLE);
  label6.setText("bpm");
  label6.setTextBold();
  label6.setOpaque(false);
  Temp = new GLabel(this, 690, 315, 120, 106);
  Temp.setTextAlign(GAlign.CENTER, GAlign.MIDDLE);
  Temp.setText("---");
  Temp.setTextBold();
  Temp.setOpaque(false);
  label5 = new GLabel(this, 653, 282, 35, 22);
  label5.setTextAlign(GAlign.RIGHT, GAlign.MIDDLE);
  label5.setText("SYS");
  label5.setTextBold();
  label5.setOpaque(false);
  label9 = new GLabel(this, 600, 305, 100, 20);
  label9.setText("Respiration");
  label9.setTextBold();
  label9.setOpaque(false);
  label7 = new GLabel(this, 698, 282, 28, 22);
  label7.setText("DIA");
  label7.setTextBold();
  label7.setOpaque(false);
  label8 = new GLabel(this, 682, 282, 15, 22);
  label8.setTextAlign(GAlign.RIGHT, GAlign.MIDDLE);
  label8.setText("/");
  label8.setTextBold();
  label8.setOpaque(false);
  rpm = new GLabel(this, 580, 315, 120, 106);
  rpm.setTextAlign(GAlign.CENTER, GAlign.MIDDLE);
  rpm.setText("---");
  rpm.setTextBold();
  rpm.setOpaque(false);
  label11 = new GLabel(this, 696, 305, 80, 20);
  label11.setText("TEMP ( ° C)");
  label11.setTextBold();
  label11.setOpaque(false);
  la = new GLabel(this, 6, 427, 84, 46);
  la.setText("Grid : OFF");
  la.setTextBold();
  la.setOpaque(false);
  label10 = new GLabel(this, 120, 427, 129, 46);
  label10.setTextAlign(GAlign.CENTER, GAlign.MIDDLE);
  label10.setText("Grid Size : 12.5 mm/s");
  label10.setTextBold();
  label10.setOpaque(false);
  grid_size = new GButton(this, 240, 420, 50, 55);
  grid_size.setLocalColorScheme(GCScheme.GREEN_SCHEME);
  grid_size.addEventHandler(this, "grid_size_click");
  gridStatus = new GButton(this, 65, 420, 50, 55);
  gridStatus.setLocalColorScheme(GCScheme.GREEN_SCHEME);
  gridStatus.addEventHandler(this, "gridStatus_click");
}

// Variable declarations 
// autogenerated do not edit
GButton record; 
GButton done; 
GButton close; 
GImageButton imgButton1; 
GLabel bpm1; 
GLabel SP02; 
GLabel BP; 
GLabel label1; 
GLabel label2; 
GLabel label3; 
GLabel label4; 
GLabel label6; 
GLabel Temp; 
GLabel label5; 
GLabel label9; 
GLabel label7; 
GLabel label8; 
GLabel rpm; 
GLabel label11; 
GLabel la; 
GLabel label10; 
GButton grid_size; 
GButton gridStatus; 