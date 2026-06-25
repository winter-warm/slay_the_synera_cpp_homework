#ifndef INTERACTCOMPONENT_H
#define INTERACTCOMPONENT_H

class InteractComponent {
public:
    bool canInteract() const { return enabled && !used; }
    bool tryInteract();
    void setEnabled(bool value) { enabled = value; }
    void reset();

private:
    bool enabled = true;
    bool used = false;
};

#endif // INTERACTCOMPONENT_H
