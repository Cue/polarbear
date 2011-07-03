import java.util.ArrayList;
import java.util.List;

/**
 * Test class that guarantees an OOM exception.
 */
public class Test {
  static class OOMList<T> extends ArrayList<T> {
  }

	public static void main(String[] args) {
		List<String> strings = new OOMList<String>();
		List<Integer> ints = new OOMList<Integer>();

    int i = 0;
    while (true) {
      strings.add("A really really long string." + i);
      ints.add(i);
      ints.add(i + 1);
    }
	}
}