# Gizmos

This module provides an API to make it easy to write *Gizmos*. A Gizmo is an
interactive bit of content, currently displayed via **block** and/or **token**.

!!! Note

*Gizmo class + Gizmo content + placement = something useful on the page.*

The idea is that gizmos provide *reusable* functionality with different
configurations that are simple for users to create.

## For the Impatient who want to get an idea of what it does.

1. Install the module and also the `gizmo_demo` module that came bundled with
   this.
2. Give 'Administer gizmos' permission to appropriate roles.
3. Visit `admin/structure/gizmo` and add a demo gizmo which simply lets you set
   a colour in its config and then generates a form in a block that will ask
   users if they like that colour too.
4. Complete the config form. Tick the 'expose block' option.
5. Place the new block on a page.
6. Visit the page and play with the form.

You can add more instances of this gizmo and include several of their blocks on
one page.


## Example: Contact Us form.

Say you have a few contact us forms to make. Maybe they each have a lot in
common but with one or two differences, such as titles, texts, whether certain
fields are required, and to which email address to send the submissions.

You could make a Contact Us gizmo which would include an admin form to allow all
the different configurations required. So a content author could create one for
project A saying "Send us your thoughts on Project A" and having those sent to
wilma@example.com, and s/he could place that on Project A's page(s). They could
also create a 2nd one for Project B. etc.

Equally it could be a donation form, or something that presents certain data
from elsewhere.

## Tell me it's already made.

I have found this functionality very useful over years of running sites and was
frustrated by the lack of anything similar in Drupal. I've ended up creating
node types (sometimes the right thing to do), tried
[Bean](https://www.drupal.org/project/bean) (too confusing for authors to use),
[Entity Creation Kit](https://www.drupal.org/project/eck) all sorts. Mostly I
had ended up doing the same thing lots of times, or repeating lots of
boilerplate which is why a reusable solution seemed handy.

