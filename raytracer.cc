#include "math.h"
#include "geometry.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <fstream>
#include <memory>


// Die folgenden Kommentare beschreiben Datenstrukturen und Funktionen
// Die Datenstrukturen und Funktionen die weiter hinten im Text beschrieben sind,
// hängen höchstens von den vorhergehenden Datenstrukturen ab, aber nicht umgekehrt.



// Ein "Bildschirm", der das Setzen eines Pixels kapselt
// Der Bildschirm hat eine Auflösung (Breite x Höhe)
// Kann zur Ausgabe einer PPM-Datei verwendet werden oder
// mit SDL2 implementiert werden.



// Eine "Kamera", die von einem Augenpunkt aus in eine Richtung senkrecht auf ein Rechteck (das Bild) zeigt.
// Für das Rechteck muss die Auflösung oder alternativ die Pixelbreite und -höhe bekannt sein.
// Für ein Pixel mit Bildkoordinate kann ein Sehstrahl erzeugt werden.



// Für die "Farbe" benötigt man nicht unbedingt eine eigene Datenstruktur.
// Sie kann als Vector3df implementiert werden mit Farbanteil von 0 bis 1.
// Vor Setzen eines Pixels auf eine bestimmte Farbe (z.B. 8-Bit-Farbtiefe),
// kann der Farbanteil mit 255 multipliziert  und der Nachkommaanteil verworfen werden.


// Das "Material" der Objektoberfläche mit ambienten, diffusem und reflektiven Farbanteil.



// Ein "Objekt", z.B. eine Kugel oder ein Dreieck, und dem zugehörigen Material der Oberfläche.
// Im Prinzip ein Wrapper-Objekt, das mindestens Material und geometrisches Objekt zusammenfasst.
// Kugel und Dreieck finden Sie in geometry.h/tcc


// verschiedene Materialdefinition, z.B. Mattes Schwarz, Mattes Rot, Reflektierendes Weiss, ...
// im wesentlichen Variablen, die mit Konstruktoraufrufen initialisiert werden.


// Die folgenden Werte zur konkreten Objekten, Lichtquellen und Funktionen, wie Lambertian-Shading
// oder die Suche nach einem Sehstrahl für das dem Augenpunkt am nächsten liegenden Objekte,
// können auch zusammen in eine Datenstruktur für die gesammte zu
// rendernde "Szene" zusammengefasst werden.

// Die Cornelbox aufgebaut aus den Objekten
// Am besten verwendet man hier einen std::vector< ... > von Objekten.

// Punktförmige "Lichtquellen" können einfach als Vector3df implementiert werden mit weisser Farbe,
// bei farbigen Lichtquellen müssen die entsprechenden Daten in Objekt zusammengefaßt werden
// Bei mehreren Lichtquellen können diese in einen std::vector gespeichert werden.

// Sie benötigen eine Implementierung von Lambertian-Shading, z.B. als Funktion
// Benötigte Werte können als Parameter übergeben werden, oder wenn diese Funktion eine Objektmethode eines
// Szene-Objekts ist, dann kann auf die Werte teilweise direkt zugegriffen werden.
// Bei mehreren Lichtquellen muss der resultierende diffuse Farbanteil durch die Anzahl Lichtquellen geteilt werden.

// Für einen Sehstrahl aus allen Objekte, dasjenige finden, das dem Augenpunkt am nächsten liegt.
// Am besten einen Zeiger auf das Objekt zurückgeben. Wenn dieser nullptr ist, dann gibt es kein sichtbares Objekt.

// Die rekursive raytracing-Methode. Am besten ab einer bestimmten Rekursionstiefe (z.B. als Parameter übergeben) abbrechen.



//Bildschirm
class Screen {
public:
  int width, height;
  std::vector<Vector3df> pixels;

  Screen(int w, int h) : width(w), height(h), pixels(w*h, Vector3df{0,0,0}) {}

  void setPixel(int x, int y, const Vector3df &color) {
    if(x < 0 || x >= width || y < 0 || y >= height) return;
    pixels[y*width + x] = color;
  }

  void savePPM(const std::string &filename) {
    std::ofstream ofs(filename);
    ofs << "P3\n" << width << " " << height << "\n255\n";
    for(auto &c : pixels) {
      int r = std::min(255, std::max(0, int(c[0]*255)));
      int g = std::min(255, std::max(0, int(c[1]*255)));
      int b = std::min(255, std::max(0, int(c[2]*255)));
      ofs << r << " " << g << " " << b << "\n";
    }
    ofs.close();
  }
};

//Kamera
class Camera {
public:
  Vector3df origin;
  Vector3df lower_left_corner;
  Vector3df horizontal;
  Vector3df vertical;

  Camera(Vector3df lookfrom, Vector3df lookat, Vector3df vup, float vfov, float aspect)
      : origin(lookfrom),
        lower_left_corner{0.0f,0.0f,0.0f},
        horizontal{0.0f,0.0f,0.0f},
        vertical{0.0f,0.0f,0.0f}
  {
    // Kamera-Koordinaten berechnen
    Vector3df w = lookfrom - lookat;
    w.normalize();
    Vector3df u = vup.cross_product(w);
    u.normalize();
    Vector3df v = w.cross_product(u);

    float theta = vfov * PI / 180.0f;
    float half_height = std::tan(theta / 2.0f);
    float half_width = aspect * half_height;

    lower_left_corner = origin - half_width * u -  half_height * v- w;
    horizontal = (2.0f * half_width) * u;
    vertical = (2.0f * half_height) * v;
  }

  // Ray für Pixelkoordinaten s,t (0..1)
  Ray3df getRay(float s, float t) const {
    Vector3df dir = lower_left_corner + s* horizontal + t* vertical - origin;
    dir.normalize();
    return Ray3df{origin, dir};
  }
};

//  Punktlichtquelle
struct PointLight {
  Vector3df position;
  Vector3df color;
};

// Material-Struktur
struct Material {
    Vector3df ambient;   // Ambientanteil
    Vector3df diffuse;   // Diffuser Farbanteil
    Vector3df specular;  // Spiegelanteil
    float shininess;
    float reflectivity;  // 0 = kein Spiegel, 1 = perfekt spiegelnd

    Material() : ambient{0,0,0}, diffuse{1,0,0}, specular{1,1,1}, shininess(32), reflectivity(0.0f) {}
    Material(Vector3df amb, Vector3df diff, Vector3df spec, float refl)
        : ambient(amb), diffuse(diff), specular(spec), shininess(32), reflectivity(refl) {}
};

//  Objekt-Wrapper
struct SceneObject {
    std::unique_ptr<Sphere3df> sphere;
    std::unique_ptr<Triangle3df> triangle;
    Material material;

    SceneObject(Sphere3df s, Material m) : sphere(std::make_unique<Sphere3df>(s)), triangle(nullptr), material(m) {}
    SceneObject(Triangle3df t, Material m) : sphere(nullptr), triangle(std::make_unique<Triangle3df>(t)), material(m) {}

    bool intersect(const Ray3df &ray, Intersection_Context<float,3> &context) const {
        if(sphere) return sphere->intersects(ray, context);
        if(triangle) return triangle->intersects(ray, context);
        return false;
    }
};

// Vector mal Vector
Vector3df multiply(const Vector3df &a, const Vector3df &b) {
    return Vector3df{a[0]*b[0], a[1]*b[1], a[2]*b[2]};
}

//Raytracing-Funktion mit Reflexion
Vector3df traceRay(
    const Ray3df &ray,
    const std::vector<SceneObject> &objects,
    //const PointLight &light,
    const std::vector<PointLight> &lights,
    int depth,
    int maxDepth = 3)

{
    if(depth > maxDepth) return Vector3df{0,0,0};

    float tClosest = 1e30f;
    Intersection_Context<float,3> closestContext;
    const SceneObject* hitObject = nullptr;
    Vector3df ambientLight = Vector3df{0.2f, 0.2f, 0.2f};


    for(const auto &obj : objects) {
        Intersection_Context<float,3> context;
        if(obj.intersect(ray, context)) {
            if(context.t < tClosest) {
                tClosest = context.t;
                closestContext = context;
                hitObject = &obj;
            }
        }
    }

    if(!hitObject) return Vector3df{0.0f,0.0f,0.0f}; // Hintergrundfarbe


    Vector3df normal = closestContext.normal;
    normal.normalize();

    // Ambient
    Vector3df color = multiply(hitObject->material.ambient, ambientLight);



    for(const auto &light : lights) {
        Vector3df toLight = light.position - closestContext.intersection;
        float distToLight = toLight.length();
        toLight.normalize();

        // Shadow-Ray
        Ray3df shadowRay{closestContext.intersection + 1e-4f * normal, toLight};
        float shadowFactor = 1.0f;
        float maxShadow = 0.5f;

        for(const auto &obj : objects) {
            if(&obj == hitObject) continue;
            Intersection_Context<float,3> shadowCtx;
            if(obj.intersect(shadowRay, shadowCtx)) {
                if(shadowCtx.t < distToLight) {
                    shadowFactor = maxShadow;
                    break;
                }
            }
        }

        // Lambert
        float diff = std::max(0.0f, normal * toLight);
        color += shadowFactor * diff * multiply(hitObject->material.diffuse, light.color);
    }


    // Rekursive Reflexion
    if(hitObject->material.reflectivity > 0.0f) {
        Vector3df D = ray.direction;
        Vector3df N = normal;
        Vector3df R = D - 2.0f * (D * N) * N;
        R.normalize();
        Ray3df reflectedRay{closestContext.intersection + 1e-3f*N, R};
        Vector3df reflectedColor = traceRay(reflectedRay, objects, lights, depth+1, maxDepth);
        //color += hitObject->material.reflectivity * reflectedColor;
        float r = hitObject->material.reflectivity;
        color = (1.0f - r) * color + r * reflectedColor;
    }


    for(int k = 0; k < 3; k++)
        color[k] = std::min(1.0f, std::max(0.0f, color[k]));

    return color;
}

int main(void) {
  // Bildschirm erstellen
  // Kamera erstellen
  // Für jede Pixelkoordinate x,y
  //   Sehstrahl für x,y mit Kamera erzeugen
  //   Farbe mit raytracing-Methode bestimmen
  //   Beim Bildschirm die Farbe für Pixel x,y, setzten

  int width = 4000;
  int height = 3000;
  Screen screen(width, height);

  // Kamera
  Camera cam(
      Vector3df{0.0f,0.0f,2.0f},
      Vector3df{0.0f,0.0f,0.0f},
      Vector3df{0.0f,1.0f,0.0f},
      90.0f,
      float(width)/height
  );

    // Objekte
    Material redMat{Vector3df{0.1f,0.0f,0.0f}, Vector3df{0.7f,0,0}, Vector3df{0,0,0}, 0.0f};
    Material greenMat{Vector3df{0.0f,0.2f,0.0f}, Vector3df{0,0.7f,0}, Vector3df{0,0,0}, 0.0f};
    Material whiteMat{Vector3df{0.1f,0.1f,0.1f}, Vector3df{0.7f,0.7f,0.7f}, Vector3df{0,0,0}, 0.0f};
    Material mirrorBallMat{Vector3df{0.0f,0.0f,0.0f}, Vector3df{1,1,1}, Vector3df{1,1,1}, 0.8f};
    Material redBallMat{Vector3df{0.1f,0,0}, Vector3df{0.9,0,0}, Vector3df{0,0,0}, 0.0f};

    std::vector<SceneObject> objects;

    // Boden (y=-1)
    objects.emplace_back(Triangle3df{Vector3df{-1,-1,-1}, Vector3df{ 1,-1,-1}, Vector3df{ 1,-1, 1}}, whiteMat);
    objects.emplace_back(Triangle3df{Vector3df{-1,-1,-1}, Vector3df{ 1,-1, 1}, Vector3df{-1,-1, 1}}, whiteMat);

    // Decke (y=1)
    objects.emplace_back(Triangle3df{Vector3df{-1,1,-1}, Vector3df{ 1,1, 1}, Vector3df{ 1,1,-1}}, whiteMat);
    objects.emplace_back(Triangle3df{Vector3df{-1,1,-1}, Vector3df{-1,1, 1}, Vector3df{ 1,1, 1}}, whiteMat);

    // Hintere Wand (z=-1)
    objects.emplace_back(Triangle3df{Vector3df{-1,-1,-1}, Vector3df{ 1,-1,-1}, Vector3df{ 1, 1,-1}}, whiteMat);
    objects.emplace_back(Triangle3df{Vector3df{-1,-1,-1}, Vector3df{ 1, 1,-1}, Vector3df{-1, 1,-1}}, whiteMat);

    // Linke Wand (x=-1)
    objects.emplace_back(Triangle3df{Vector3df{-1,-1,-1}, Vector3df{-1, 1,-1}, Vector3df{-1, 1, 1}}, redMat);
    objects.emplace_back(Triangle3df{Vector3df{-1,-1,-1}, Vector3df{-1, 1, 1}, Vector3df{-1,-1, 1}}, redMat);

    // Rechte Wand (x=1)
    objects.emplace_back(Triangle3df{Vector3df{1,-1,-1}, Vector3df{1, 1, 1}, Vector3df{1, 1,-1}}, greenMat);
    objects.emplace_back(Triangle3df{Vector3df{1,-1,-1}, Vector3df{1,-1, 1}, Vector3df{1, 1, 1}}, greenMat);

    // Kugeln innerhalb der Box
    objects.emplace_back(Sphere3df{Vector3df{-0.4f,-0.6f,-0.4f},0.3f}, mirrorBallMat);
    objects.emplace_back(Sphere3df{Vector3df{0.4f,-0.8f,0.2f},0.2f}, redBallMat);


  //Lichtquelle

    std::vector<PointLight> lights;
    lights.push_back(PointLight{Vector3df{0.0f,0.9f,0.0f}, Vector3df{1.f,1.f,1.f}});

    // Pixel-Loop
    for(int j=0;j<height;j++) {
        for(int i=0;i<width;i++) {
            float u = float(i)/width;
            float v = 1- float(j)/height;
            Ray3df ray = cam.getRay(u,v);
            Vector3df pixelColor = traceRay(ray, objects, lights, 0, 3);
            screen.setPixel(i, height-j-1, pixelColor);
        }
    }

  screen.savePPM("output.ppm");
  std::cout << "Fertig! output.ppm \n";
  return 0;
}

