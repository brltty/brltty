public class Apitest {
  public static void main(String argv[]) {
    try {
      Brlapi brlapi = new Brlapi();
      int fd;
      BrlapiSize size;
      int tty;
      BrlapiKey key;
      long keylong;
      Brlapi.settings settings = new Brlapi.settings();
      Brlapi.writeStruct ws2 = new Brlapi.writeStruct(-1, 10, 20,
	  "Key Pressed         ",
	  "????????????????????".getBytes(),
	  null,3);
      System.out.print("Connecting to BrlAPI... ");
      System.loadLibrary("brlapi_java");
      fd = brlapi.initializeConnection(settings, settings);
      size = brlapi.getDisplaySize();
      System.out.println("done (fd="+fd+")");
      System.out.println("Connected to "+settings.hostName+" using key file "+settings.authKey);
      System.out.println("driver "+brlapi.getDriverId()+" "+brlapi.getDriverName());
      System.out.println("display "+size.x()+"x"+size.y());
      tty = brlapi.enterTtyMode(0,null);
      System.out.println("got tty "+tty);
      brlapi.writeText(0,"ok !!");
      keylong = brlapi.readKey(true);
      key = new BrlapiKey(keylong);
      System.out.println("got key "+keylong+": ("+key.command+","+key.argument+","+key.flags+")");
      brlapi.write(ws2);
      keylong = brlapi.readKey(true);
      key = new BrlapiKey(keylong);
      System.out.println("got key "+keylong+": ("+key.command+","+key.argument+","+key.flags+")");
      brlapi.leaveTtyMode();
      brlapi.closeConnection();
    } catch (BrlapiError e) {
      System.out.println("got error " + e);
    }
  }
}
